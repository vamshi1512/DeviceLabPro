#include "devicelab/simulator.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace devicelab {

namespace {

constexpr std::uint64_t kFnvOffset = 14695981039346656037ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;

bool is_critical_fault(const FaultType type) {
    return type == FaultType::SensorFailure || type == FaultType::Overheating;
}

bool is_soft_fault(const FaultType type) {
    return type == FaultType::Timeout || type == FaultType::CommunicationFailure;
}

double clamp_value(const double value, const double minimum, const double maximum) {
    return std::max(minimum, std::min(maximum, value));
}

double approach_value(const double current, const double target, const double max_step) {
    if (current < target) {
        return std::min(current + max_step, target);
    }
    return std::max(current - max_step, target);
}

std::uint64_t mix64(std::uint64_t value) {
    value ^= value >> 33u;
    value *= 0xff51afd7ed558ccdull;
    value ^= value >> 33u;
    value *= 0xc4ceb9fe1a85ec53ull;
    value ^= value >> 33u;
    return value;
}

double deterministic_noise(const std::uint32_t seed, const Tick tick, const std::uint64_t channel, const double amplitude) {
    if (amplitude <= 0.0) {
        return 0.0;
    }
    const std::uint64_t raw = mix64(
        static_cast<std::uint64_t>(seed) * 1315423911ull ^ (tick + 1ull) * 2654435761ull ^
        (channel + 1ull) * 11400714819323198485ull);
    const double normalized = static_cast<double>(raw % 1000000ull) / 500000.0 - 1.0;
    return normalized * amplitude;
}

std::uint64_t fnv1a_update(std::uint64_t state, const std::string_view value) {
    for (const unsigned char ch : value) {
        state ^= ch;
        state *= kFnvPrime;
    }
    return state;
}

std::string hex_digest(const std::uint64_t value) {
    std::ostringstream output;
    output << std::hex << std::setfill('0') << std::setw(16) << value;
    return output.str();
}

std::string slugify(const std::string& input) {
    std::string output;
    output.reserve(input.size());
    bool last_dash = false;
    for (const unsigned char ch : input) {
        if (std::isalnum(ch)) {
            output.push_back(static_cast<char>(std::tolower(ch)));
            last_dash = false;
        } else if (!last_dash) {
            output.push_back('-');
            last_dash = true;
        }
    }
    while (!output.empty() && output.front() == '-') {
        output.erase(output.begin());
    }
    while (!output.empty() && output.back() == '-') {
        output.pop_back();
    }
    return output.empty() ? std::string{"run"} : output;
}

std::string join_active_fault_names(const std::vector<FaultType>& faults) {
    if (faults.empty()) {
        return "none";
    }
    std::ostringstream output;
    for (std::size_t index = 0; index < faults.size(); ++index) {
        if (index != 0) {
            output << ", ";
        }
        output << to_string(faults[index]);
    }
    return output.str();
}

std::string build_digest(
    const std::vector<TelemetryFrame>& telemetry,
    const std::vector<LogEntry>& logs,
    const std::vector<StateTransition>& transitions) {
    std::uint64_t digest = kFnvOffset;
    for (const auto& frame : telemetry) {
        digest = fnv1a_update(digest, json(frame).dump());
    }
    for (const auto& log : logs) {
        digest = fnv1a_update(digest, json(log).dump());
    }
    for (const auto& transition : transitions) {
        digest = fnv1a_update(digest, json(transition).dump());
    }
    return hex_digest(digest);
}

bool log_contains_before_tick(const std::vector<LogEntry>& logs, const Tick tick, const std::string& needle) {
    return std::any_of(logs.begin(), logs.end(), [&](const LogEntry& entry) {
        return entry.tick <= tick && entry.message.find(needle) != std::string::npos;
    });
}

double read_actuator_numeric(const ActuatorState& actuators, const std::string& field) {
    if (field == "fan_pwm") {
        return static_cast<double>(actuators.fan_pwm);
    }
    if (field == "valve_position") {
        return static_cast<double>(actuators.valve_position);
    }
    throw std::runtime_error("Unsupported actuator numeric field: " + field);
}

bool read_actuator_boolean(const ActuatorState& actuators, const std::string& field) {
    if (field == "heater_enabled") {
        return actuators.heater_enabled;
    }
    if (field == "pump_enabled") {
        return actuators.pump_enabled;
    }
    if (field == "alarm_buzzer") {
        return actuators.alarm_buzzer;
    }
    throw std::runtime_error("Unsupported actuator boolean field: " + field);
}

json build_summary_json(
    const ScenarioDefinition& scenario,
    const DeviceProfile& profile,
    const RunArtifacts& artifacts) {
    json assertions = json::array();
    for (const auto& assertion : artifacts.assertions) {
        assertions.push_back(json{
            {"tick", assertion.tick},
            {"passed", assertion.passed},
            {"description", assertion.description},
        });
    }

    json transitions = json::array();
    for (const auto& transition : artifacts.transitions) {
        transitions.push_back(transition);
    }

    json active_faults = json::array();
    if (!artifacts.telemetry.empty()) {
        for (const auto fault : artifacts.telemetry.back().active_faults) {
            active_faults.push_back(to_string(fault));
        }
    }

    const auto passed_assertions = std::count_if(
        artifacts.assertions.begin(),
        artifacts.assertions.end(),
        [](const ScenarioAssertionResult& assertion) { return assertion.passed; });

    return json{
        {"format_version", 1},
        {"run_id", artifacts.run_id},
        {"scenario", scenario_definition_to_json(scenario)},
        {"profile", device_profile_to_json(profile)},
        {"passed", artifacts.passed},
        {"seed", artifacts.seed},
        {"duration_ticks", artifacts.duration_ticks},
        {"digest", artifacts.digest},
        {"telemetry_count", artifacts.telemetry.size()},
        {"log_count", artifacts.logs.size()},
        {"transition_count", artifacts.transitions.size()},
        {"assertion_pass_count", passed_assertions},
        {"assertion_fail_count", artifacts.assertions.size() - passed_assertions},
        {"final_state", artifacts.telemetry.empty() ? "OFF" : to_string(artifacts.telemetry.back().state)},
        {"final_faults", active_faults},
        {"assertions", assertions},
        {"transitions", transitions},
    };
}

ScenarioAssertionResult evaluate_expectation(
    const Expectation& expectation,
    const TelemetryFrame& frame,
    const std::vector<LogEntry>& logs) {
    ScenarioAssertionResult result;
    result.tick = expectation.tick;

    auto fail = [&](const std::string& description) {
        result.passed = false;
        result.description = description;
    };
    auto pass = [&](const std::string& description) {
        result.passed = true;
        result.description = description;
    };

    switch (expectation.kind) {
    case ExpectationKind::StateEquals: {
        const auto expected = enum_from_string<DeviceState>(expectation.string_value);
        if (frame.state == expected) {
            pass("State matched expected value " + expectation.string_value);
        } else {
            fail(
                "Expected state " + expectation.string_value + " but observed " + to_string(frame.state));
        }
        break;
    }
    case ExpectationKind::FaultPresent: {
        const auto expected = enum_from_string<FaultType>(expectation.string_value);
        const bool present = std::find(frame.active_faults.begin(), frame.active_faults.end(), expected) !=
                             frame.active_faults.end();
        if (present) {
            pass("Fault " + expectation.string_value + " present");
        } else {
            fail("Expected fault " + expectation.string_value + " to be active");
        }
        break;
    }
    case ExpectationKind::FaultAbsent: {
        const auto expected = enum_from_string<FaultType>(expectation.string_value);
        const bool present = std::find(frame.active_faults.begin(), frame.active_faults.end(), expected) !=
                             frame.active_faults.end();
        if (!present) {
            pass("Fault " + expectation.string_value + " absent");
        } else {
            fail("Expected fault " + expectation.string_value + " to be cleared");
        }
        break;
    }
    case ExpectationKind::ActuatorEquals: {
        if (expectation.field == "heater_enabled" || expectation.field == "pump_enabled" ||
            expectation.field == "alarm_buzzer") {
            const bool actual = read_actuator_boolean(frame.actuators, expectation.field);
            if (actual == expectation.boolean_value) {
                pass("Actuator " + expectation.field + " matched expected boolean");
            } else {
                fail("Actuator " + expectation.field + " boolean mismatch");
            }
        } else {
            const double actual = read_actuator_numeric(frame.actuators, expectation.field);
            if (std::fabs(actual - expectation.numeric_value) < 0.001) {
                pass("Actuator " + expectation.field + " matched expected numeric value");
            } else {
                fail("Actuator " + expectation.field + " expected " + std::to_string(expectation.numeric_value) +
                     " but observed " + std::to_string(actual));
            }
        }
        break;
    }
    case ExpectationKind::ActuatorAtLeast: {
        const double actual = read_actuator_numeric(frame.actuators, expectation.field);
        if (actual >= expectation.numeric_value) {
            pass("Actuator " + expectation.field + " satisfied minimum value");
        } else {
            fail("Actuator " + expectation.field + " below minimum threshold");
        }
        break;
    }
    case ExpectationKind::TemperatureBelow: {
        if (frame.sensors.temperature_c < expectation.numeric_value) {
            pass("Temperature below threshold");
        } else {
            fail("Temperature exceeded maximum threshold");
        }
        break;
    }
    case ExpectationKind::TemperatureAbove: {
        if (frame.sensors.temperature_c > expectation.numeric_value) {
            pass("Temperature above threshold");
        } else {
            fail("Temperature did not cross minimum threshold");
        }
        break;
    }
    case ExpectationKind::LogContains: {
        if (log_contains_before_tick(logs, expectation.tick, expectation.string_value)) {
            pass("Observed expected log marker");
        } else {
            fail("Did not observe log marker containing '" + expectation.string_value + "'");
        }
        break;
    }
    }

    return result;
}

} // namespace

DeviceSimulator::DeviceSimulator(DeviceProfile profile, const std::uint32_t seed)
    : profile_(std::move(profile)), seed_(seed) {
    validate_device_profile(profile_);
    target_temperature_c_ = profile_.target_temperature_c;
    sensors_.temperature_c = profile_.initial_temperature_c;
    sensors_.pressure_kpa = profile_.nominal_pressure_kpa;
    sensors_.flow_lpm = 0.0;
    sensors_.link_ok = true;
    sensors_.valid = true;

    for (const auto type : {FaultType::SensorFailure, FaultType::Timeout, FaultType::Overheating,
                            FaultType::CommunicationFailure}) {
        faults_.emplace(type, FaultRecord{type, false, false, "", 0, 0});
    }

    log(
        Severity::Info,
        EventType::Log,
        "Simulator initialized",
        json{{"profile", profile_.id}, {"seed", seed_}, {"target_temperature_c", target_temperature_c_}});
}

Tick DeviceSimulator::current_tick() const {
    return tick_;
}

DeviceState DeviceSimulator::state() const {
    return state_;
}

const SensorSample& DeviceSimulator::sensors() const {
    return sensors_;
}

const ActuatorState& DeviceSimulator::actuators() const {
    return actuators_;
}

double DeviceSimulator::target_temperature() const {
    return target_temperature_c_;
}

const std::vector<LogEntry>& DeviceSimulator::logs() const {
    return logs_;
}

const std::vector<StateTransition>& DeviceSimulator::transitions() const {
    return transitions_;
}

std::vector<FaultRecord> DeviceSimulator::active_faults() const {
    std::vector<FaultRecord> active;
    for (const auto& [_, fault] : faults_) {
        if (fault.active) {
            active.push_back(fault);
        }
    }
    return active;
}

bool DeviceSimulator::is_fault_active(const FaultType type) const {
    const auto it = faults_.find(type);
    return it != faults_.end() && it->second.active;
}

void DeviceSimulator::set_scenario_label(std::string label) {
    scenario_label_ = std::move(label);
}

void DeviceSimulator::apply_command(const Command& command) {
    log(
        Severity::Info,
        EventType::Log,
        "Command processed",
        json{{"command", to_string(command.type)}, {"source", command.source}, {"tick", tick_}});

    switch (command.type) {
    case CommandType::PowerOn:
        if (state_ == DeviceState::Off) {
            transition_to(DeviceState::Booting, "Power on command");
            last_heartbeat_tick_ = tick_;
        }
        break;
    case CommandType::PowerOff:
        cycle_requested_ = false;
        if (state_ != DeviceState::Off) {
            transition_to(DeviceState::Shutdown, "Power off command");
        }
        break;
    case CommandType::StartCycle:
        cycle_requested_ = true;
        last_heartbeat_tick_ = tick_;
        if (state_ == DeviceState::Idle && active_faults().empty()) {
            transition_to(DeviceState::Running, "Start cycle command");
        }
        break;
    case CommandType::StopCycle:
        cycle_requested_ = false;
        if (state_ == DeviceState::Running || state_ == DeviceState::Degraded) {
            transition_to(DeviceState::Idle, "Stop cycle command");
        }
        break;
    case CommandType::AckFault:
        if (state_ == DeviceState::Faulted && !is_fault_active(FaultType::SensorFailure) &&
            !is_fault_active(FaultType::Overheating)) {
            transition_to(DeviceState::Recovering, "Fault acknowledged");
        }
        break;
    case CommandType::Reset:
        cycle_requested_ = false;
        sensors_.valid = true;
        sensors_.link_ok = true;
        last_heartbeat_tick_ = tick_;
        for (auto& [type, fault] : faults_) {
            if (fault.active) {
                clear_fault_internal(type, "Reset command");
            }
        }
        if (state_ != DeviceState::Off) {
            transition_to(DeviceState::Booting, "Reset command");
        }
        break;
    case CommandType::Heartbeat:
        last_heartbeat_tick_ = tick_;
        if (is_fault_active(FaultType::Timeout) && !faults_.at(FaultType::Timeout).injected) {
            clear_fault_internal(FaultType::Timeout, "Heartbeat restored");
        }
        break;
    case CommandType::SetTargetTemperature:
        if (command.value.has_value()) {
            target_temperature_c_ = clamp_value(
                *command.value,
                profile_.ambient_temperature_c + 5.0,
                profile_.critical_temperature_c - 2.0);
            log(
                Severity::Info,
                EventType::Log,
                "Target temperature updated",
                json{{"target_temperature_c", target_temperature_c_}});
        }
        break;
    case CommandType::RequestSnapshot:
        log(Severity::Debug, EventType::Log, "Snapshot requested", snapshot_json());
        break;
    }
}

void DeviceSimulator::apply_sensor_override(const SensorOverride& sensor_override) {
    if (sensor_override.temperature_c.has_value()) {
        sensors_.temperature_c = *sensor_override.temperature_c;
    }
    if (sensor_override.pressure_kpa.has_value()) {
        sensors_.pressure_kpa = *sensor_override.pressure_kpa;
    }
    if (sensor_override.flow_lpm.has_value()) {
        sensors_.flow_lpm = *sensor_override.flow_lpm;
    }
    if (sensor_override.link_ok.has_value()) {
        sensors_.link_ok = *sensor_override.link_ok;
    }
    if (sensor_override.valid.has_value()) {
        sensors_.valid = *sensor_override.valid;
    }
    log(
        Severity::Info,
        EventType::Log,
        "Sensor override applied",
        json{{"override", sensor_override}});
}

void DeviceSimulator::inject_fault(const FaultType type, const std::string& reason, const bool injected) {
    raise_fault(type, reason, injected);
}

void DeviceSimulator::clear_fault(const FaultType type, const std::string& reason) {
    if (type == FaultType::CommunicationFailure) {
        sensors_.link_ok = true;
    }
    if (type == FaultType::SensorFailure) {
        sensors_.valid = true;
    }
    if (type == FaultType::Timeout) {
        last_heartbeat_tick_ = tick_;
    }
    clear_fault_internal(type, reason);
}

void DeviceSimulator::add_marker(const std::string& note) {
    log(Severity::Info, EventType::ScenarioMarker, note, json{{"tick", tick_}});
}

TelemetryFrame DeviceSimulator::advance_tick() {
    evaluate_automatic_faults();
    normalize_fault_recovery();
    update_control_loop();
    update_sensor_model();
    evaluate_automatic_faults();
    normalize_fault_recovery();
    update_control_loop();

    TelemetryFrame frame;
    frame.tick = tick_;
    frame.state = state_;
    frame.sensors = sensors_;
    frame.actuators = actuators_;
    frame.target_temperature_c = target_temperature_c_;
    frame.progress_pct = progress_percentage();
    frame.scenario_label = scenario_label_;

    for (const auto& [fault_type, fault] : faults_) {
        if (fault.active) {
            frame.active_faults.push_back(fault_type);
        }
    }
    frame.notes = json::array();
    if (state_ == DeviceState::Degraded) {
        frame.notes.push_back("Reduced capability mode");
    }
    if (state_ == DeviceState::Faulted) {
        frame.notes.push_back("Safe-state outputs active");
    }

    ++tick_;
    return frame;
}

json DeviceSimulator::snapshot_json() const {
    json active = json::array();
    for (const auto& [fault_type, fault] : faults_) {
        if (fault.active) {
            active.push_back(to_string(fault_type));
        }
    }

    return json{
        {"tick", tick_},
        {"state", to_string(state_)},
        {"sensors", sensors_},
        {"actuators", actuators_},
        {"target_temperature_c", target_temperature_c_},
        {"cycle_requested", cycle_requested_},
        {"active_faults", active},
        {"scenario_label", scenario_label_},
    };
}

void DeviceSimulator::transition_to(const DeviceState next, const std::string& reason) {
    if (next == state_) {
        return;
    }
    transitions_.push_back(StateTransition{tick_, state_, next, reason});
    log(
        Severity::Info,
        EventType::StateTransition,
        "State transition",
        json{{"from", to_string(state_)}, {"to", to_string(next)}, {"reason", reason}});
    state_ = next;
    state_entered_tick_ = tick_;
    if (next == DeviceState::Degraded) {
        degraded_since_tick_ = tick_;
    }
}

void DeviceSimulator::raise_fault(const FaultType type, const std::string& reason, const bool injected) {
    auto& fault = faults_.at(type);
    if (fault.active) {
        return;
    }
    fault.active = true;
    fault.injected = injected;
    fault.reason = reason;
    fault.raised_at = tick_;
    fault.cleared_at = 0;
    if (type == FaultType::SensorFailure) {
        sensors_.valid = false;
    }
    if (type == FaultType::CommunicationFailure) {
        sensors_.link_ok = false;
    }

    log(
        is_critical_fault(type) ? Severity::Critical : Severity::Error,
        EventType::FaultRaised,
        "Fault raised",
        json{{"fault", to_string(type)}, {"reason", reason}, {"injected", injected}});
}

void DeviceSimulator::clear_fault_internal(const FaultType type, const std::string& reason) {
    auto& fault = faults_.at(type);
    if (!fault.active) {
        return;
    }
    fault.active = false;
    fault.reason = reason;
    fault.cleared_at = tick_;

    log(
        Severity::Info,
        EventType::FaultCleared,
        "Fault cleared",
        json{{"fault", to_string(type)}, {"reason", reason}});
}

void DeviceSimulator::log(
    const Severity severity,
    const EventType event_type,
    const std::string& message,
    json context) {
    logs_.push_back(LogEntry{tick_, severity, event_type, message, std::move(context)});
}

void DeviceSimulator::evaluate_automatic_faults() {
    if (!sensors_.valid) {
        raise_fault(FaultType::SensorFailure, "Sensor validity dropped", false);
    } else if (is_fault_active(FaultType::SensorFailure) && !faults_.at(FaultType::SensorFailure).injected) {
        clear_fault_internal(FaultType::SensorFailure, "Sensor validity restored");
    }

    if (!sensors_.link_ok) {
        raise_fault(FaultType::CommunicationFailure, "Telemetry link lost", false);
    } else if (is_fault_active(FaultType::CommunicationFailure) &&
               !faults_.at(FaultType::CommunicationFailure).injected) {
        clear_fault_internal(FaultType::CommunicationFailure, "Telemetry link restored");
    }

    if (sensors_.temperature_c >= profile_.critical_temperature_c) {
        raise_fault(FaultType::Overheating, "Critical temperature threshold crossed", false);
    } else if (is_fault_active(FaultType::Overheating) && !faults_.at(FaultType::Overheating).injected &&
               sensors_.temperature_c <= profile_.recovery_temperature_c) {
        clear_fault_internal(FaultType::Overheating, "Temperature returned to recovery band");
    }

    const bool timeout_due =
        state_ == DeviceState::Running || state_ == DeviceState::Degraded || state_ == DeviceState::Recovering;
    if (timeout_due && tick_ > last_heartbeat_tick_ + profile_.heartbeat_timeout_ticks) {
        raise_fault(FaultType::Timeout, "Heartbeat timeout elapsed", false);
    } else if (is_fault_active(FaultType::Timeout) && !faults_.at(FaultType::Timeout).injected &&
               tick_ <= last_heartbeat_tick_ + profile_.heartbeat_timeout_ticks) {
        clear_fault_internal(FaultType::Timeout, "Heartbeat window restored");
    }
}

void DeviceSimulator::update_control_loop() {
    switch (state_) {
    case DeviceState::Off:
        actuators_ = ActuatorState{};
        break;
    case DeviceState::Booting:
        actuators_.heater_enabled = false;
        actuators_.pump_enabled = false;
        actuators_.fan_pwm = 18;
        actuators_.valve_position = 10;
        actuators_.alarm_buzzer = false;
        break;
    case DeviceState::Idle:
        actuators_.heater_enabled = false;
        actuators_.pump_enabled = false;
        actuators_.fan_pwm = sensors_.temperature_c > profile_.ambient_temperature_c + 6.0 ? 42 : 24;
        actuators_.valve_position = 12;
        actuators_.alarm_buzzer = false;
        break;
    case DeviceState::Running: {
        const bool needs_heat = sensors_.temperature_c < target_temperature_c_ - 1.2;
        const bool overshoot = sensors_.temperature_c > target_temperature_c_ + 1.5;
        actuators_.heater_enabled = needs_heat;
        actuators_.pump_enabled = true;
        actuators_.fan_pwm = overshoot ? profile_.boosted_fan_pwm : profile_.nominal_fan_pwm;
        actuators_.valve_position = needs_heat ? 78 : 58;
        actuators_.alarm_buzzer = false;
        break;
    }
    case DeviceState::Degraded:
        actuators_.heater_enabled = cycle_requested_ && sensors_.temperature_c < target_temperature_c_ - 4.0;
        actuators_.pump_enabled = true;
        actuators_.fan_pwm = profile_.boosted_fan_pwm;
        actuators_.valve_position = 48;
        actuators_.alarm_buzzer = false;
        break;
    case DeviceState::Faulted:
        actuators_.heater_enabled = false;
        actuators_.pump_enabled = sensors_.temperature_c > profile_.recovery_temperature_c;
        actuators_.fan_pwm = profile_.boosted_fan_pwm;
        actuators_.valve_position = 30;
        actuators_.alarm_buzzer = true;
        break;
    case DeviceState::Recovering:
        actuators_.heater_enabled = false;
        actuators_.pump_enabled = sensors_.temperature_c > profile_.ambient_temperature_c + 2.0;
        actuators_.fan_pwm = profile_.boosted_fan_pwm;
        actuators_.valve_position = 38;
        actuators_.alarm_buzzer = false;
        break;
    case DeviceState::Shutdown:
        actuators_.heater_enabled = false;
        actuators_.pump_enabled = false;
        actuators_.fan_pwm = 10;
        actuators_.valve_position = 0;
        actuators_.alarm_buzzer = false;
        break;
    }
}

void DeviceSimulator::update_sensor_model() {
    const double fan_factor = static_cast<double>(actuators_.fan_pwm) / 100.0;
    const double cooling_step = profile_.cooling_per_tick * (0.35 + fan_factor * 0.85);

    if (actuators_.heater_enabled) {
        const double heat_gain = state_ == DeviceState::Degraded ? profile_.heat_gain_per_tick * 0.55
                                                                 : profile_.heat_gain_per_tick;
        sensors_.temperature_c += heat_gain - profile_.cooling_per_tick * 0.2;
    } else {
        sensors_.temperature_c =
            approach_value(sensors_.temperature_c, profile_.ambient_temperature_c, cooling_step);
    }

    if (actuators_.pump_enabled) {
        const double desired_flow = state_ == DeviceState::Degraded ? profile_.pump_flow_degraded_lpm
                                                                    : profile_.pump_flow_nominal_lpm;
        const double desired_pressure =
            state_ == DeviceState::Running ? profile_.nominal_pressure_kpa + 4.0 : profile_.nominal_pressure_kpa - 1.0;
        sensors_.flow_lpm = approach_value(sensors_.flow_lpm, desired_flow, 4.0);
        sensors_.pressure_kpa = approach_value(sensors_.pressure_kpa, desired_pressure, 3.2);
    } else {
        sensors_.flow_lpm = approach_value(sensors_.flow_lpm, 0.0, profile_.pressure_drop_per_tick);
        sensors_.pressure_kpa =
            approach_value(sensors_.pressure_kpa, profile_.nominal_pressure_kpa - 2.5, 2.0);
    }

    if (is_fault_active(FaultType::CommunicationFailure)) {
        sensors_.link_ok = false;
        sensors_.flow_lpm = std::max(0.0, sensors_.flow_lpm - 2.5);
    } else {
        sensors_.link_ok = true;
    }

    if (is_fault_active(FaultType::Timeout)) {
        sensors_.pressure_kpa -= 0.8;
        sensors_.flow_lpm = std::max(0.0, sensors_.flow_lpm - 1.5);
    }

    if (is_fault_active(FaultType::SensorFailure)) {
        sensors_.valid = false;
        sensors_.flow_lpm = 0.0;
    } else {
        sensors_.valid = true;
    }

    sensors_.temperature_c += deterministic_noise(seed_, tick_, 0, profile_.sensor_noise_temperature_c);
    sensors_.pressure_kpa += deterministic_noise(seed_, tick_, 1, profile_.sensor_noise_pressure_kpa);

    sensors_.pressure_kpa = clamp_value(sensors_.pressure_kpa, profile_.min_pressure_kpa - 10.0, profile_.max_pressure_kpa + 10.0);
    sensors_.flow_lpm = clamp_value(sensors_.flow_lpm, 0.0, profile_.pump_flow_nominal_lpm + 2.0);
}

void DeviceSimulator::normalize_fault_recovery() {
    const bool critical_fault =
        is_fault_active(FaultType::SensorFailure) || is_fault_active(FaultType::Overheating);
    const bool soft_fault =
        is_fault_active(FaultType::Timeout) || is_fault_active(FaultType::CommunicationFailure);

    if (critical_fault) {
        if (state_ != DeviceState::Faulted) {
            transition_to(DeviceState::Faulted, "Critical fault asserted");
        }
        return;
    }

    if (state_ == DeviceState::Faulted) {
        return;
    }

    if (state_ == DeviceState::Shutdown &&
        tick_ >= state_entered_tick_ + profile_.shutdown_ticks) {
        transition_to(DeviceState::Off, "Shutdown complete");
        return;
    }

    if (state_ == DeviceState::Booting &&
        tick_ >= state_entered_tick_ + profile_.boot_ticks) {
        if (soft_fault) {
            transition_to(DeviceState::Degraded, "Boot completed with active soft fault");
        } else if (cycle_requested_) {
            transition_to(DeviceState::Running, "Boot completed into active cycle");
        } else {
            transition_to(DeviceState::Idle, "Boot completed");
        }
        return;
    }

    if (state_ == DeviceState::Recovering) {
        if (!soft_fault && sensors_.temperature_c <= profile_.recovery_temperature_c &&
            tick_ >= state_entered_tick_ + profile_.recovery_ticks) {
            transition_to(cycle_requested_ ? DeviceState::Running : DeviceState::Idle, "Recovery complete");
        }
        return;
    }

    if ((state_ == DeviceState::Running || state_ == DeviceState::Idle) && soft_fault) {
        transition_to(DeviceState::Degraded, "Soft fault asserted");
        return;
    }

    if (state_ == DeviceState::Running && !soft_fault &&
        sensors_.temperature_c >= profile_.warning_temperature_c) {
        transition_to(DeviceState::Degraded, "Thermal warning threshold crossed");
        return;
    }

    if (state_ == DeviceState::Degraded) {
        if (soft_fault && tick_ >= degraded_since_tick_ + profile_.degraded_escalation_ticks) {
            transition_to(DeviceState::Faulted, "Soft fault escalation window elapsed");
            return;
        }

        if (!soft_fault && sensors_.temperature_c < profile_.warning_temperature_c - 1.0) {
            transition_to(cycle_requested_ ? DeviceState::Running : DeviceState::Idle, "Degraded condition cleared");
            return;
        }
    }

    if (state_ == DeviceState::Running && !cycle_requested_) {
        transition_to(DeviceState::Idle, "Cycle request cleared");
        return;
    }

    if (state_ == DeviceState::Idle && cycle_requested_ && !soft_fault) {
        transition_to(DeviceState::Running, "Cycle request active");
    }
}

double DeviceSimulator::progress_percentage() const {
    if (state_ == DeviceState::Off) {
        return 0.0;
    }
    if (state_ == DeviceState::Booting) {
        const double elapsed = static_cast<double>(tick_ - state_entered_tick_);
        return clamp_value(100.0 * elapsed / static_cast<double>(std::max<Tick>(1, profile_.boot_ticks)), 0.0, 100.0);
    }
    if (state_ == DeviceState::Running || state_ == DeviceState::Degraded) {
        const double denominator = std::max(1.0, target_temperature_c_ - profile_.ambient_temperature_c);
        return clamp_value(
            100.0 * (sensors_.temperature_c - profile_.ambient_temperature_c) / denominator,
            0.0,
            100.0);
    }
    if (state_ == DeviceState::Recovering) {
        const double denominator = std::max(1.0, profile_.critical_temperature_c - profile_.recovery_temperature_c);
        return clamp_value(
            100.0 * (profile_.critical_temperature_c - sensors_.temperature_c) / denominator,
            0.0,
            100.0);
    }
    return 100.0;
}

RunArtifacts run_scenario(
    const ScenarioDefinition& scenario,
    const DeviceProfile& profile,
    const std::filesystem::path& output_dir) {
    validate_scenario_definition(scenario);
    validate_device_profile(profile);

    DeviceSimulator simulator(profile, scenario.seed);
    simulator.set_scenario_label(scenario.name);

    RunArtifacts artifacts;
    artifacts.scenario_id = scenario.id;
    artifacts.scenario_name = scenario.name;
    artifacts.profile_id = profile.id;
    artifacts.seed = scenario.seed;
    artifacts.duration_ticks = scenario.duration_ticks;

    std::size_t event_index = 0;
    std::size_t expectation_index = 0;

    for (Tick tick = 0; tick < scenario.duration_ticks; ++tick) {
        while (event_index < scenario.events.size() && scenario.events[event_index].tick == tick) {
            const auto& event = scenario.events[event_index];
            switch (event.type) {
            case ScenarioEventType::Command:
                if (event.command.has_value()) {
                    simulator.apply_command(*event.command);
                }
                break;
            case ScenarioEventType::SensorOverride:
                if (event.sensor_override.has_value()) {
                    simulator.apply_sensor_override(*event.sensor_override);
                }
                break;
            case ScenarioEventType::InjectFault:
                if (event.fault.has_value()) {
                    simulator.inject_fault(
                        *event.fault,
                        event.note.empty() ? "Scenario injected fault" : event.note,
                        true);
                }
                break;
            case ScenarioEventType::ClearFault:
                if (event.fault.has_value()) {
                    simulator.clear_fault(
                        *event.fault,
                        event.note.empty() ? "Scenario cleared fault" : event.note);
                }
                break;
            case ScenarioEventType::Note:
                simulator.add_marker(event.note);
                break;
            }
            ++event_index;
        }

        const auto frame = simulator.advance_tick();
        artifacts.telemetry.push_back(frame);

        while (expectation_index < scenario.expectations.size() &&
               scenario.expectations[expectation_index].tick == tick) {
            auto assertion =
                evaluate_expectation(scenario.expectations[expectation_index], frame, simulator.logs());
            artifacts.assertions.push_back(assertion);
            if (!assertion.passed) {
                artifacts.passed = false;
            }
            ++expectation_index;
        }
    }

    artifacts.logs = simulator.logs();
    artifacts.transitions = simulator.transitions();
    artifacts.digest = build_digest(artifacts.telemetry, artifacts.logs, artifacts.transitions);
    artifacts.run_id = slugify(scenario.id) + "-" + artifacts.digest.substr(0, 12);
    artifacts.summary = build_summary_json(scenario, profile, artifacts);

    if (!output_dir.empty()) {
        write_run_artifacts(scenario, profile, artifacts, output_dir);
    }
    return artifacts;
}

RunArtifacts replay_run(
    const std::filesystem::path& replay_manifest_path,
    const std::filesystem::path& output_dir) {
    std::ifstream input(replay_manifest_path);
    if (!input) {
        throw std::runtime_error("Unable to open replay manifest: " + replay_manifest_path.string());
    }

    json manifest;
    input >> manifest;

    auto scenario = scenario_definition_from_json(manifest.at("scenario"));
    auto profile = device_profile_from_json(manifest.at("profile"));
    if (manifest.contains("seed")) {
        scenario.seed = manifest.at("seed").get<std::uint32_t>();
    }

    auto artifacts = run_scenario(scenario, profile, {});

    if (manifest.contains("expected_digest")) {
        const auto expected = manifest.at("expected_digest").get<std::string>();
        ScenarioAssertionResult assertion;
        assertion.tick = artifacts.duration_ticks ? artifacts.duration_ticks - 1 : 0;
        if (artifacts.digest == expected) {
            assertion.passed = true;
            assertion.description = "Replay digest matched expected deterministic digest";
        } else {
            assertion.passed = false;
            assertion.description =
                "Replay digest mismatch. Expected " + expected + " but observed " + artifacts.digest;
            artifacts.passed = false;
        }
        artifacts.assertions.push_back(assertion);
        artifacts.summary = build_summary_json(scenario, profile, artifacts);
    }

    if (!output_dir.empty()) {
        write_run_artifacts(scenario, profile, artifacts, output_dir);
    }
    return artifacts;
}

void write_run_artifacts(
    const ScenarioDefinition& scenario,
    const DeviceProfile& profile,
    const RunArtifacts& artifacts,
    const std::filesystem::path& output_dir) {
    std::filesystem::create_directories(output_dir);

    {
        std::ofstream output(output_dir / "summary.json");
        output << artifacts.summary.dump(2) << '\n';
    }

    {
        std::ofstream output(output_dir / "telemetry.jsonl");
        for (const auto& frame : artifacts.telemetry) {
            output << json(frame).dump() << '\n';
        }
    }

    {
        std::ofstream output(output_dir / "logs.jsonl");
        for (const auto& entry : artifacts.logs) {
            output << json(entry).dump() << '\n';
        }
    }

    {
        std::ofstream output(output_dir / "transitions.json");
        json transitions = json::array();
        for (const auto& transition : artifacts.transitions) {
            transitions.push_back(transition);
        }
        output << transitions.dump(2) << '\n';
    }

    {
        json manifest{
            {"format_version", 1},
            {"seed", artifacts.seed},
            {"expected_digest", artifacts.digest},
            {"scenario", scenario_definition_to_json(scenario)},
            {"profile", device_profile_to_json(profile)},
        };
        std::ofstream output(output_dir / "replay_manifest.json");
        output << manifest.dump(2) << '\n';
    }

    {
        std::ofstream output(output_dir / "report.md");
        output << "# DeviceLab Pro Run Report\n\n";
        output << "- Run ID: `" << artifacts.run_id << "`\n";
        output << "- Scenario: `" << scenario.id << "` (" << scenario.name << ")\n";
        output << "- Profile: `" << profile.id << "` (" << profile.name << ")\n";
        output << "- Result: " << (artifacts.passed ? "PASS" : "FAIL") << '\n';
        output << "- Deterministic Digest: `" << artifacts.digest << "`\n";
        output << "- Final State: `"
               << (artifacts.telemetry.empty() ? "OFF" : to_string(artifacts.telemetry.back().state)) << "`\n";
        output << "- Active Faults: "
               << (artifacts.telemetry.empty() ? "none"
                                               : join_active_fault_names(
                                                     artifacts.telemetry.back().active_faults))
               << "\n\n";

        output << "## Scenario Summary\n\n";
        output << scenario.description << "\n\n";

        output << "## Assertions\n\n";
        for (const auto& assertion : artifacts.assertions) {
            output << "- [" << (assertion.passed ? 'x' : ' ') << "] tick " << assertion.tick << ": "
                   << assertion.description << '\n';
        }
        if (artifacts.assertions.empty()) {
            output << "- No explicit assertions defined.\n";
        }
        output << '\n';

        output << "## State Transitions\n\n";
        for (const auto& transition : artifacts.transitions) {
            output << "- tick " << transition.tick << ": `" << to_string(transition.from) << "` -> `"
                   << to_string(transition.to) << "` (" << transition.reason << ")\n";
        }
        if (artifacts.transitions.empty()) {
            output << "- No state transitions captured.\n";
        }
        output << '\n';

        output << "## Final Snapshot\n\n```json\n";
        if (!artifacts.telemetry.empty()) {
            output << json(artifacts.telemetry.back()).dump(2);
        } else {
            output << "{}";
        }
        output << "\n```\n";
    }
}

} // namespace devicelab
