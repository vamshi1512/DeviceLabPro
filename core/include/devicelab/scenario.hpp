#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "devicelab/device_profile.hpp"

namespace devicelab {

enum class ScenarioEventType {
    Command,
    SensorOverride,
    InjectFault,
    ClearFault,
    Note,
};

enum class ExpectationKind {
    StateEquals,
    FaultPresent,
    FaultAbsent,
    ActuatorEquals,
    ActuatorAtLeast,
    TemperatureBelow,
    TemperatureAbove,
    LogContains,
};

struct ScenarioEvent {
    Tick tick = 0;
    ScenarioEventType type = ScenarioEventType::Note;
    std::optional<Command> command;
    std::optional<SensorOverride> sensor_override;
    std::optional<FaultType> fault;
    std::string note;
};

struct Expectation {
    Tick tick = 0;
    ExpectationKind kind = ExpectationKind::StateEquals;
    std::string field;
    std::string string_value;
    double numeric_value = 0.0;
    bool boolean_value = false;
};

struct ScenarioDefinition {
    std::string id;
    std::string name;
    std::string description;
    std::string profile_id;
    Tick duration_ticks = 0;
    std::uint32_t seed = 7;
    bool deterministic = true;
    std::vector<ScenarioEvent> events;
    std::vector<Expectation> expectations;
};

std::string to_string(ScenarioEventType type);
std::string to_string(ExpectationKind kind);
ScenarioDefinition load_scenario_definition(const std::filesystem::path& scenario_path);
ScenarioDefinition scenario_definition_from_json(const json& j);
void validate_scenario_definition(const ScenarioDefinition& scenario);
json scenario_definition_to_json(const ScenarioDefinition& scenario);

} // namespace devicelab
