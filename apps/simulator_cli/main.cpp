#include <algorithm>
#include <chrono>
#include <cctype>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "devicelab/simulator.hpp"

namespace devicelab {
namespace {

std::vector<std::string> collect_args(const int argc, char** argv) {
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc));
    for (int index = 0; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }
    return args;
}

std::optional<std::string> option_value(const std::vector<std::string>& args, const std::string& name) {
    for (std::size_t index = 0; index + 1 < args.size(); ++index) {
        if (args[index] == name) {
            return args[index + 1];
        }
    }
    return std::nullopt;
}

bool has_flag(const std::vector<std::string>& args, const std::string& name) {
    return std::find(args.begin(), args.end(), name) != args.end();
}

std::string timestamp_token() {
    const auto now = std::time(nullptr);
    const auto local_time = *std::localtime(&now);
    std::ostringstream output;
    output << std::put_time(&local_time, "%Y%m%d-%H%M%S");
    return output.str();
}

std::filesystem::path default_output_dir(const std::filesystem::path& source, const std::string& prefix = "") {
    const auto stem = source.stem().string();
    const auto label = prefix.empty() ? stem : prefix + "-" + stem;
    return std::filesystem::path("runs") / (label + "-" + timestamp_token());
}

std::string normalize_symbol(std::string value) {
    for (char& ch : value) {
        if (ch == '-' || ch == ' ') {
            ch = '_';
        } else {
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        }
    }
    return value;
}

void print_usage() {
    std::cout
        << "DeviceLab Pro CLI\n\n"
        << "Commands:\n"
        << "  run --scenario <file> --profile <file> [--output-dir <dir>]\n"
        << "  replay --manifest <file> [--output-dir <dir>]\n"
        << "  stream --scenario <file> --profile <file> [--output-dir <dir>] [--delay-ms <n>]\n"
        << "  interactive --profile <file>\n"
        << "  list [--scenarios-dir <dir>] [--profiles-dir <dir>]\n";
}

void print_directory_listing(const std::filesystem::path& directory, const std::string& label) {
    std::cout << label << ":\n";
    if (!std::filesystem::exists(directory)) {
        std::cout << "  (missing) " << directory.string() << "\n";
        return;
    }
    std::vector<std::filesystem::path> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    for (const auto& path : files) {
        std::cout << "  - " << path.filename().string() << "\n";
    }
}

Command command_from_token(const std::string& token, const std::optional<double> value = std::nullopt) {
    Command command;
    command.type = enum_from_string<CommandType>(normalize_symbol(token));
    command.source = "interactive";
    command.value = value;
    return command;
}

FaultType fault_from_token(const std::string& token) {
    return enum_from_string<FaultType>(normalize_symbol(token));
}

void run_stream_command(
    const std::filesystem::path& scenario_path,
    const std::filesystem::path& profile_path,
    const std::filesystem::path& output_dir,
    const int delay_ms) {
    const auto scenario = load_scenario_definition(scenario_path);
    const auto profile = load_device_profile(profile_path);
    const auto artifacts = run_scenario(scenario, profile, output_dir);

    std::size_t log_index = 0;
    std::size_t transition_index = 0;

    for (const auto& frame : artifacts.telemetry) {
        std::cout << json{{"type", "telemetry"}, {"payload", frame}}.dump() << '\n';

        while (log_index < artifacts.logs.size() && artifacts.logs[log_index].tick == frame.tick) {
            std::cout << json{{"type", "log"}, {"payload", artifacts.logs[log_index]}}.dump() << '\n';
            ++log_index;
        }

        while (transition_index < artifacts.transitions.size() &&
               artifacts.transitions[transition_index].tick == frame.tick) {
            std::cout << json{{"type", "transition"}, {"payload", artifacts.transitions[transition_index]}}.dump()
                      << '\n';
            ++transition_index;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
    }

    std::cout << json{{"type", "summary"}, {"payload", artifacts.summary}}.dump() << '\n';
}

void run_interactive_command(const std::filesystem::path& profile_path) {
    DeviceSimulator simulator(load_device_profile(profile_path));

    std::cout << "Interactive mode. Commands: power_on, power_off, start_cycle, stop_cycle, ack_fault,\n"
              << "reset, heartbeat, set_target <temp>, inject <fault>, clear <fault>,\n"
              << "sensor temperature=.. pressure=.. flow=.. link_ok=true valid=true, tick [count], status, quit\n";

    std::string line;
    while (std::cout << "devicelab> " && std::getline(std::cin, line)) {
        std::istringstream input(line);
        std::string action;
        input >> action;
        if (action.empty()) {
            continue;
        }
        if (action == "quit" || action == "exit") {
            break;
        }
        if (action == "status") {
            std::cout << simulator.snapshot_json().dump(2) << '\n';
            continue;
        }
        if (action == "tick") {
            int count = 1;
            input >> count;
            for (int index = 0; index < count; ++index) {
                std::cout << json(simulator.advance_tick()).dump(2) << '\n';
            }
            continue;
        }
        if (action == "inject" || action == "clear") {
            std::string fault_name;
            input >> fault_name;
            if (fault_name.empty()) {
                std::cerr << "Fault name required\n";
                continue;
            }
            if (action == "inject") {
                simulator.inject_fault(fault_from_token(fault_name), "Interactive injection", true);
            } else {
                simulator.clear_fault(fault_from_token(fault_name), "Interactive clear");
            }
            continue;
        }
        if (action == "sensor") {
            SensorOverride sensor_override;
            std::string token;
            while (input >> token) {
                const auto separator = token.find('=');
                if (separator == std::string::npos) {
                    continue;
                }
                const auto key = token.substr(0, separator);
                const auto value = token.substr(separator + 1);
                if (key == "temperature") {
                    sensor_override.temperature_c = std::stod(value);
                } else if (key == "pressure") {
                    sensor_override.pressure_kpa = std::stod(value);
                } else if (key == "flow") {
                    sensor_override.flow_lpm = std::stod(value);
                } else if (key == "link_ok") {
                    sensor_override.link_ok = value == "true";
                } else if (key == "valid") {
                    sensor_override.valid = value == "true";
                }
            }
            simulator.apply_sensor_override(sensor_override);
            continue;
        }
        if (action == "set_target") {
            double target = 0.0;
            input >> target;
            simulator.apply_command(command_from_token("set_target_temperature", target));
            continue;
        }

        try {
            simulator.apply_command(command_from_token(action));
        } catch (const std::exception& error) {
            std::cerr << "Unknown command: " << error.what() << '\n';
        }
    }
}

} // namespace
} // namespace devicelab

int main(int argc, char** argv) {
    using namespace devicelab;

    const auto args = collect_args(argc, argv);
    if (args.size() < 2 || has_flag(args, "--help")) {
        print_usage();
        return 0;
    }

    const auto command = args[1];

    try {
        if (command == "run") {
            const auto scenario_path = option_value(args, "--scenario");
            const auto profile_path = option_value(args, "--profile");
            if (!scenario_path.has_value() || !profile_path.has_value()) {
                throw std::runtime_error("run requires --scenario and --profile");
            }
            const auto output_dir =
                option_value(args, "--output-dir").value_or(default_output_dir(*scenario_path).string());
            const auto artifacts = run_scenario(
                load_scenario_definition(*scenario_path),
                load_device_profile(*profile_path),
                output_dir);
            std::cout << artifacts.summary.dump(2) << '\n';
            return artifacts.passed ? 0 : 2;
        }

        if (command == "replay") {
            const auto manifest_path = option_value(args, "--manifest");
            if (!manifest_path.has_value()) {
                throw std::runtime_error("replay requires --manifest");
            }
            const auto output_dir = option_value(args, "--output-dir")
                                        .value_or(default_output_dir(*manifest_path, "replay").string());
            const auto artifacts = replay_run(*manifest_path, output_dir);
            std::cout << artifacts.summary.dump(2) << '\n';
            return artifacts.passed ? 0 : 2;
        }

        if (command == "stream") {
            const auto scenario_path = option_value(args, "--scenario");
            const auto profile_path = option_value(args, "--profile");
            const auto delay_ms = option_value(args, "--delay-ms").value_or("160");
            if (!scenario_path.has_value() || !profile_path.has_value()) {
                throw std::runtime_error("stream requires --scenario and --profile");
            }
            const auto output_dir = option_value(args, "--output-dir")
                                        .value_or(default_output_dir(*scenario_path, "stream").string());
            run_stream_command(*scenario_path, *profile_path, output_dir, std::stoi(delay_ms));
            return 0;
        }

        if (command == "interactive") {
            const auto profile_path = option_value(args, "--profile");
            if (!profile_path.has_value()) {
                throw std::runtime_error("interactive requires --profile");
            }
            run_interactive_command(*profile_path);
            return 0;
        }

        if (command == "list") {
            const auto scenarios_dir = option_value(args, "--scenarios-dir").value_or("scenarios");
            const auto profiles_dir = option_value(args, "--profiles-dir").value_or("profiles");
            print_directory_listing(scenarios_dir, "Scenarios");
            print_directory_listing(profiles_dir, "Profiles");
            return 0;
        }

        throw std::runtime_error("Unknown command: " + command);
    } catch (const std::exception& error) {
        std::cerr << "DeviceLab Pro CLI error: " << error.what() << '\n';
        return 1;
    }
}
