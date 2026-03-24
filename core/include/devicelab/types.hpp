#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <nlohmann/json.hpp>

namespace devicelab {

using json = nlohmann::json;
using Tick = std::uint64_t;

enum class DeviceState {
    Off,
    Booting,
    Idle,
    Running,
    Degraded,
    Faulted,
    Recovering,
    Shutdown,
};

enum class CommandType {
    PowerOn,
    PowerOff,
    StartCycle,
    StopCycle,
    AckFault,
    Reset,
    Heartbeat,
    SetTargetTemperature,
    RequestSnapshot,
};

enum class FaultType {
    SensorFailure,
    Timeout,
    Overheating,
    CommunicationFailure,
};

enum class Severity {
    Debug,
    Info,
    Warning,
    Error,
    Critical,
};

enum class EventType {
    Log,
    Telemetry,
    StateTransition,
    FaultRaised,
    FaultCleared,
    ScenarioMarker,
    ScenarioResult,
};

struct SensorSample {
    double temperature_c = 24.0;
    double pressure_kpa = 101.3;
    double flow_lpm = 0.0;
    bool link_ok = true;
    bool valid = true;
};

struct SensorOverride {
    std::optional<double> temperature_c;
    std::optional<double> pressure_kpa;
    std::optional<double> flow_lpm;
    std::optional<bool> link_ok;
    std::optional<bool> valid;
};

struct ActuatorState {
    bool heater_enabled = false;
    bool pump_enabled = false;
    int fan_pwm = 0;
    int valve_position = 0;
    bool alarm_buzzer = false;
};

struct Command {
    CommandType type = CommandType::RequestSnapshot;
    std::optional<double> value;
    std::string source = "cli";
};

struct FaultRecord {
    FaultType type = FaultType::Timeout;
    bool active = false;
    bool injected = false;
    std::string reason;
    Tick raised_at = 0;
    Tick cleared_at = 0;
};

struct LogEntry {
    Tick tick = 0;
    Severity severity = Severity::Info;
    EventType event_type = EventType::Log;
    std::string message;
    json context = json::object();
};

struct TelemetryFrame {
    Tick tick = 0;
    DeviceState state = DeviceState::Off;
    SensorSample sensors;
    ActuatorState actuators;
    std::vector<FaultType> active_faults;
    double target_temperature_c = 60.0;
    double progress_pct = 0.0;
    std::string scenario_label;
    json notes = json::array();
};

struct StateTransition {
    Tick tick = 0;
    DeviceState from = DeviceState::Off;
    DeviceState to = DeviceState::Off;
    std::string reason;
};

inline std::string to_string(DeviceState state) {
    switch (state) {
    case DeviceState::Off:
        return "OFF";
    case DeviceState::Booting:
        return "BOOTING";
    case DeviceState::Idle:
        return "IDLE";
    case DeviceState::Running:
        return "RUNNING";
    case DeviceState::Degraded:
        return "DEGRADED";
    case DeviceState::Faulted:
        return "FAULTED";
    case DeviceState::Recovering:
        return "RECOVERING";
    case DeviceState::Shutdown:
        return "SHUTDOWN";
    }
    throw std::runtime_error("Unknown device state");
}

inline std::string to_string(CommandType type) {
    switch (type) {
    case CommandType::PowerOn:
        return "POWER_ON";
    case CommandType::PowerOff:
        return "POWER_OFF";
    case CommandType::StartCycle:
        return "START_CYCLE";
    case CommandType::StopCycle:
        return "STOP_CYCLE";
    case CommandType::AckFault:
        return "ACK_FAULT";
    case CommandType::Reset:
        return "RESET";
    case CommandType::Heartbeat:
        return "HEARTBEAT";
    case CommandType::SetTargetTemperature:
        return "SET_TARGET_TEMPERATURE";
    case CommandType::RequestSnapshot:
        return "REQUEST_SNAPSHOT";
    }
    throw std::runtime_error("Unknown command type");
}

inline std::string to_string(FaultType type) {
    switch (type) {
    case FaultType::SensorFailure:
        return "SENSOR_FAILURE";
    case FaultType::Timeout:
        return "TIMEOUT";
    case FaultType::Overheating:
        return "OVERHEATING";
    case FaultType::CommunicationFailure:
        return "COMMUNICATION_FAILURE";
    }
    throw std::runtime_error("Unknown fault type");
}

inline std::string to_string(Severity severity) {
    switch (severity) {
    case Severity::Debug:
        return "DEBUG";
    case Severity::Info:
        return "INFO";
    case Severity::Warning:
        return "WARNING";
    case Severity::Error:
        return "ERROR";
    case Severity::Critical:
        return "CRITICAL";
    }
    throw std::runtime_error("Unknown severity");
}

inline std::string to_string(EventType type) {
    switch (type) {
    case EventType::Log:
        return "LOG";
    case EventType::Telemetry:
        return "TELEMETRY";
    case EventType::StateTransition:
        return "STATE_TRANSITION";
    case EventType::FaultRaised:
        return "FAULT_RAISED";
    case EventType::FaultCleared:
        return "FAULT_CLEARED";
    case EventType::ScenarioMarker:
        return "SCENARIO_MARKER";
    case EventType::ScenarioResult:
        return "SCENARIO_RESULT";
    }
    throw std::runtime_error("Unknown event type");
}

template <typename EnumType>
inline EnumType enum_from_string(const std::string& value);

template <>
inline DeviceState enum_from_string<DeviceState>(const std::string& value) {
    if (value == "OFF") {
        return DeviceState::Off;
    }
    if (value == "BOOTING") {
        return DeviceState::Booting;
    }
    if (value == "IDLE") {
        return DeviceState::Idle;
    }
    if (value == "RUNNING") {
        return DeviceState::Running;
    }
    if (value == "DEGRADED") {
        return DeviceState::Degraded;
    }
    if (value == "FAULTED") {
        return DeviceState::Faulted;
    }
    if (value == "RECOVERING") {
        return DeviceState::Recovering;
    }
    if (value == "SHUTDOWN") {
        return DeviceState::Shutdown;
    }
    throw std::runtime_error("Invalid device state: " + value);
}

template <>
inline CommandType enum_from_string<CommandType>(const std::string& value) {
    if (value == "POWER_ON") {
        return CommandType::PowerOn;
    }
    if (value == "POWER_OFF") {
        return CommandType::PowerOff;
    }
    if (value == "START_CYCLE") {
        return CommandType::StartCycle;
    }
    if (value == "STOP_CYCLE") {
        return CommandType::StopCycle;
    }
    if (value == "ACK_FAULT") {
        return CommandType::AckFault;
    }
    if (value == "RESET") {
        return CommandType::Reset;
    }
    if (value == "HEARTBEAT") {
        return CommandType::Heartbeat;
    }
    if (value == "SET_TARGET_TEMPERATURE") {
        return CommandType::SetTargetTemperature;
    }
    if (value == "REQUEST_SNAPSHOT") {
        return CommandType::RequestSnapshot;
    }
    throw std::runtime_error("Invalid command type: " + value);
}

template <>
inline FaultType enum_from_string<FaultType>(const std::string& value) {
    if (value == "SENSOR_FAILURE") {
        return FaultType::SensorFailure;
    }
    if (value == "TIMEOUT") {
        return FaultType::Timeout;
    }
    if (value == "OVERHEATING") {
        return FaultType::Overheating;
    }
    if (value == "COMMUNICATION_FAILURE") {
        return FaultType::CommunicationFailure;
    }
    throw std::runtime_error("Invalid fault type: " + value);
}

template <>
inline Severity enum_from_string<Severity>(const std::string& value) {
    if (value == "DEBUG") {
        return Severity::Debug;
    }
    if (value == "INFO") {
        return Severity::Info;
    }
    if (value == "WARNING") {
        return Severity::Warning;
    }
    if (value == "ERROR") {
        return Severity::Error;
    }
    if (value == "CRITICAL") {
        return Severity::Critical;
    }
    throw std::runtime_error("Invalid severity: " + value);
}

template <>
inline EventType enum_from_string<EventType>(const std::string& value) {
    if (value == "LOG") {
        return EventType::Log;
    }
    if (value == "TELEMETRY") {
        return EventType::Telemetry;
    }
    if (value == "STATE_TRANSITION") {
        return EventType::StateTransition;
    }
    if (value == "FAULT_RAISED") {
        return EventType::FaultRaised;
    }
    if (value == "FAULT_CLEARED") {
        return EventType::FaultCleared;
    }
    if (value == "SCENARIO_MARKER") {
        return EventType::ScenarioMarker;
    }
    if (value == "SCENARIO_RESULT") {
        return EventType::ScenarioResult;
    }
    throw std::runtime_error("Invalid event type: " + value);
}

inline void to_json(json& j, const SensorSample& sample) {
    j = json{
        {"temperature_c", sample.temperature_c},
        {"pressure_kpa", sample.pressure_kpa},
        {"flow_lpm", sample.flow_lpm},
        {"link_ok", sample.link_ok},
        {"valid", sample.valid},
    };
}

inline void from_json(const json& j, SensorSample& sample) {
    sample.temperature_c = j.value("temperature_c", sample.temperature_c);
    sample.pressure_kpa = j.value("pressure_kpa", sample.pressure_kpa);
    sample.flow_lpm = j.value("flow_lpm", sample.flow_lpm);
    sample.link_ok = j.value("link_ok", sample.link_ok);
    sample.valid = j.value("valid", sample.valid);
}

inline void to_json(json& j, const SensorOverride& sample) {
    j = json::object();
    if (sample.temperature_c.has_value()) {
        j["temperature_c"] = *sample.temperature_c;
    }
    if (sample.pressure_kpa.has_value()) {
        j["pressure_kpa"] = *sample.pressure_kpa;
    }
    if (sample.flow_lpm.has_value()) {
        j["flow_lpm"] = *sample.flow_lpm;
    }
    if (sample.link_ok.has_value()) {
        j["link_ok"] = *sample.link_ok;
    }
    if (sample.valid.has_value()) {
        j["valid"] = *sample.valid;
    }
}

inline void from_json(const json& j, SensorOverride& sample) {
    if (j.contains("temperature_c")) {
        sample.temperature_c = j.at("temperature_c").get<double>();
    }
    if (j.contains("pressure_kpa")) {
        sample.pressure_kpa = j.at("pressure_kpa").get<double>();
    }
    if (j.contains("flow_lpm")) {
        sample.flow_lpm = j.at("flow_lpm").get<double>();
    }
    if (j.contains("link_ok")) {
        sample.link_ok = j.at("link_ok").get<bool>();
    }
    if (j.contains("valid")) {
        sample.valid = j.at("valid").get<bool>();
    }
}

inline void to_json(json& j, const ActuatorState& actuators) {
    j = json{
        {"heater_enabled", actuators.heater_enabled},
        {"pump_enabled", actuators.pump_enabled},
        {"fan_pwm", actuators.fan_pwm},
        {"valve_position", actuators.valve_position},
        {"alarm_buzzer", actuators.alarm_buzzer},
    };
}

inline void from_json(const json& j, ActuatorState& actuators) {
    actuators.heater_enabled = j.value("heater_enabled", actuators.heater_enabled);
    actuators.pump_enabled = j.value("pump_enabled", actuators.pump_enabled);
    actuators.fan_pwm = j.value("fan_pwm", actuators.fan_pwm);
    actuators.valve_position = j.value("valve_position", actuators.valve_position);
    actuators.alarm_buzzer = j.value("alarm_buzzer", actuators.alarm_buzzer);
}

inline void to_json(json& j, const Command& command) {
    j = json{
        {"type", to_string(command.type)},
        {"source", command.source},
    };
    if (command.value.has_value()) {
        j["value"] = *command.value;
    }
}

inline void from_json(const json& j, Command& command) {
    command.type = enum_from_string<CommandType>(j.at("type").get<std::string>());
    command.source = j.value("source", std::string{"scenario"});
    if (j.contains("value")) {
        command.value = j.at("value").get<double>();
    }
}

inline void to_json(json& j, const FaultRecord& fault) {
    j = json{
        {"type", to_string(fault.type)},
        {"active", fault.active},
        {"injected", fault.injected},
        {"reason", fault.reason},
        {"raised_at", fault.raised_at},
        {"cleared_at", fault.cleared_at},
    };
}

inline void to_json(json& j, const LogEntry& entry) {
    j = json{
        {"tick", entry.tick},
        {"severity", to_string(entry.severity)},
        {"event_type", to_string(entry.event_type)},
        {"message", entry.message},
        {"context", entry.context},
    };
}

inline void to_json(json& j, const TelemetryFrame& frame) {
    j = json{
        {"tick", frame.tick},
        {"state", to_string(frame.state)},
        {"sensors", frame.sensors},
        {"actuators", frame.actuators},
        {"target_temperature_c", frame.target_temperature_c},
        {"progress_pct", frame.progress_pct},
        {"scenario_label", frame.scenario_label},
        {"notes", frame.notes},
    };
    j["active_faults"] = json::array();
    for (const auto fault : frame.active_faults) {
        j["active_faults"].push_back(to_string(fault));
    }
}

inline void to_json(json& j, const StateTransition& transition) {
    j = json{
        {"tick", transition.tick},
        {"from", to_string(transition.from)},
        {"to", to_string(transition.to)},
        {"reason", transition.reason},
    };
}

} // namespace devicelab
