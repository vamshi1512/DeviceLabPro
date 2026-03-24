#include "devicelab/scenario.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace devicelab {

namespace {

bool is_boolean_actuator_field(const std::string& value) {
    return value == "heater_enabled" || value == "pump_enabled" || value == "alarm_buzzer";
}

bool is_numeric_actuator_field(const std::string& value) {
    return value == "fan_pwm" || value == "valve_position";
}

ScenarioEventType scenario_event_type_from_string(const std::string& value) {
    if (value == "command") {
        return ScenarioEventType::Command;
    }
    if (value == "sensor_override") {
        return ScenarioEventType::SensorOverride;
    }
    if (value == "inject_fault") {
        return ScenarioEventType::InjectFault;
    }
    if (value == "clear_fault") {
        return ScenarioEventType::ClearFault;
    }
    if (value == "note") {
        return ScenarioEventType::Note;
    }
    throw std::runtime_error("Invalid scenario event type: " + value);
}

ExpectationKind expectation_kind_from_string(const std::string& value) {
    if (value == "state_equals") {
        return ExpectationKind::StateEquals;
    }
    if (value == "fault_present") {
        return ExpectationKind::FaultPresent;
    }
    if (value == "fault_absent") {
        return ExpectationKind::FaultAbsent;
    }
    if (value == "actuator_equals") {
        return ExpectationKind::ActuatorEquals;
    }
    if (value == "actuator_at_least") {
        return ExpectationKind::ActuatorAtLeast;
    }
    if (value == "temperature_below") {
        return ExpectationKind::TemperatureBelow;
    }
    if (value == "temperature_above") {
        return ExpectationKind::TemperatureAbove;
    }
    if (value == "log_contains") {
        return ExpectationKind::LogContains;
    }
    throw std::runtime_error("Invalid expectation kind: " + value);
}

ScenarioEvent scenario_event_from_json(const json& j) {
    ScenarioEvent event;
    event.tick = j.at("tick").get<Tick>();
    event.type = scenario_event_type_from_string(j.at("type").get<std::string>());
    event.note = j.value("note", std::string{});

    if (j.contains("command")) {
        event.command = j.at("command").get<Command>();
    }
    if (j.contains("sensor_override")) {
        event.sensor_override = j.at("sensor_override").get<SensorOverride>();
    }
    if (j.contains("fault")) {
        event.fault = enum_from_string<FaultType>(j.at("fault").get<std::string>());
    }
    return event;
}

Expectation expectation_from_json(const json& j) {
    Expectation expectation;
    expectation.tick = j.at("tick").get<Tick>();
    expectation.kind = expectation_kind_from_string(j.at("kind").get<std::string>());
    expectation.field = j.value("field", std::string{});

    if (j.contains("value")) {
        if (j.at("value").is_boolean()) {
            expectation.boolean_value = j.at("value").get<bool>();
        } else if (j.at("value").is_number()) {
            expectation.numeric_value = j.at("value").get<double>();
        } else if (j.at("value").is_string()) {
            expectation.string_value = j.at("value").get<std::string>();
        }
    }
    if (j.contains("fault")) {
        expectation.string_value = j.at("fault").get<std::string>();
    }
    if (j.contains("contains")) {
        expectation.string_value = j.at("contains").get<std::string>();
    }
    return expectation;
}

} // namespace

ScenarioDefinition scenario_definition_from_json(const json& j) {
    ScenarioDefinition scenario;
    scenario.id = j.at("id").get<std::string>();
    scenario.name = j.at("name").get<std::string>();
    scenario.description = j.value("description", std::string{});
    scenario.profile_id = j.value("profile_id", std::string{});
    scenario.duration_ticks = j.at("duration_ticks").get<Tick>();
    scenario.seed = j.value("seed", scenario.seed);
    scenario.deterministic = j.value("deterministic", true);

    for (const auto& event_json : j.value("events", json::array())) {
        scenario.events.push_back(scenario_event_from_json(event_json));
    }
    for (const auto& expectation_json : j.value("expectations", json::array())) {
        scenario.expectations.push_back(expectation_from_json(expectation_json));
    }

    std::sort(
        scenario.events.begin(),
        scenario.events.end(),
        [](const ScenarioEvent& left, const ScenarioEvent& right) { return left.tick < right.tick; });
    std::sort(
        scenario.expectations.begin(),
        scenario.expectations.end(),
        [](const Expectation& left, const Expectation& right) { return left.tick < right.tick; });
    validate_scenario_definition(scenario);
    return scenario;
}

void validate_scenario_definition(const ScenarioDefinition& scenario) {
    if (scenario.id.empty()) {
        throw std::runtime_error("Scenario id must not be empty");
    }
    if (scenario.name.empty()) {
        throw std::runtime_error("Scenario name must not be empty");
    }
    if (scenario.duration_ticks == 0) {
        throw std::runtime_error("Scenario duration_ticks must be greater than zero");
    }

    for (const auto& event : scenario.events) {
        if (event.tick >= scenario.duration_ticks) {
            throw std::runtime_error("Scenario event tick exceeds scenario duration");
        }
        switch (event.type) {
        case ScenarioEventType::Command:
            if (!event.command.has_value()) {
                throw std::runtime_error("Command event must contain a command payload");
            }
            break;
        case ScenarioEventType::SensorOverride:
            if (!event.sensor_override.has_value()) {
                throw std::runtime_error("Sensor override event must contain a sensor_override payload");
            }
            break;
        case ScenarioEventType::InjectFault:
        case ScenarioEventType::ClearFault:
            if (!event.fault.has_value()) {
                throw std::runtime_error("Fault event must contain a fault payload");
            }
            break;
        case ScenarioEventType::Note:
            if (event.note.empty()) {
                throw std::runtime_error("Note event must contain a note");
            }
            break;
        }
    }

    for (const auto& expectation : scenario.expectations) {
        if (expectation.tick >= scenario.duration_ticks) {
            throw std::runtime_error("Scenario expectation tick exceeds scenario duration");
        }
        switch (expectation.kind) {
        case ExpectationKind::StateEquals:
            if (expectation.string_value.empty()) {
                throw std::runtime_error("state_equals expectation must contain a state value");
            }
            static_cast<void>(enum_from_string<DeviceState>(expectation.string_value));
            break;
        case ExpectationKind::FaultPresent:
        case ExpectationKind::FaultAbsent:
            if (expectation.string_value.empty()) {
                throw std::runtime_error("fault expectation must contain a fault value");
            }
            static_cast<void>(enum_from_string<FaultType>(expectation.string_value));
            break;
        case ExpectationKind::ActuatorEquals:
            if (!(is_boolean_actuator_field(expectation.field) || is_numeric_actuator_field(expectation.field))) {
                throw std::runtime_error("actuator expectation uses an unsupported field");
            }
            break;
        case ExpectationKind::ActuatorAtLeast:
            if (!is_numeric_actuator_field(expectation.field)) {
                throw std::runtime_error("actuator_at_least requires a numeric actuator field");
            }
            break;
        case ExpectationKind::TemperatureBelow:
        case ExpectationKind::TemperatureAbove:
            break;
        case ExpectationKind::LogContains:
            if (expectation.string_value.empty()) {
                throw std::runtime_error("log_contains expectation must contain text");
            }
            break;
        }
    }
}

std::string to_string(ScenarioEventType type) {
    switch (type) {
    case ScenarioEventType::Command:
        return "command";
    case ScenarioEventType::SensorOverride:
        return "sensor_override";
    case ScenarioEventType::InjectFault:
        return "inject_fault";
    case ScenarioEventType::ClearFault:
        return "clear_fault";
    case ScenarioEventType::Note:
        return "note";
    }
    throw std::runtime_error("Unknown scenario event type");
}

std::string to_string(ExpectationKind kind) {
    switch (kind) {
    case ExpectationKind::StateEquals:
        return "state_equals";
    case ExpectationKind::FaultPresent:
        return "fault_present";
    case ExpectationKind::FaultAbsent:
        return "fault_absent";
    case ExpectationKind::ActuatorEquals:
        return "actuator_equals";
    case ExpectationKind::ActuatorAtLeast:
        return "actuator_at_least";
    case ExpectationKind::TemperatureBelow:
        return "temperature_below";
    case ExpectationKind::TemperatureAbove:
        return "temperature_above";
    case ExpectationKind::LogContains:
        return "log_contains";
    }
    throw std::runtime_error("Unknown expectation kind");
}

ScenarioDefinition load_scenario_definition(const std::filesystem::path& scenario_path) {
    std::ifstream input(scenario_path);
    if (!input) {
        throw std::runtime_error("Unable to open scenario: " + scenario_path.string());
    }

    json j;
    input >> j;
    return scenario_definition_from_json(j);
}

json scenario_definition_to_json(const ScenarioDefinition& scenario) {
    json events = json::array();
    for (const auto& event : scenario.events) {
        json item{
            {"tick", event.tick},
            {"type", to_string(event.type)},
            {"note", event.note},
        };
        if (event.command.has_value()) {
            item["command"] = *event.command;
        }
        if (event.sensor_override.has_value()) {
            item["sensor_override"] = *event.sensor_override;
        }
        if (event.fault.has_value()) {
            item["fault"] = to_string(*event.fault);
        }
        events.push_back(item);
    }

    json expectations = json::array();
    for (const auto& expectation : scenario.expectations) {
        json item{
            {"tick", expectation.tick},
            {"kind", to_string(expectation.kind)},
            {"field", expectation.field},
        };
        if (!expectation.string_value.empty()) {
            if (expectation.kind == ExpectationKind::FaultPresent ||
                expectation.kind == ExpectationKind::FaultAbsent) {
                item["fault"] = expectation.string_value;
            } else if (expectation.kind == ExpectationKind::LogContains) {
                item["contains"] = expectation.string_value;
            } else {
                item["value"] = expectation.string_value;
            }
        } else if (is_boolean_actuator_field(expectation.field)) {
            item["value"] = expectation.boolean_value;
        } else {
            item["value"] = expectation.numeric_value;
        }
        expectations.push_back(item);
    }

    return json{
        {"id", scenario.id},
        {"name", scenario.name},
        {"description", scenario.description},
        {"profile_id", scenario.profile_id},
        {"duration_ticks", scenario.duration_ticks},
        {"seed", scenario.seed},
        {"deterministic", scenario.deterministic},
        {"events", events},
        {"expectations", expectations},
    };
}

} // namespace devicelab
