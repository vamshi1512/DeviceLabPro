#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include "devicelab/scenario.hpp"

namespace devicelab {

struct ScenarioAssertionResult {
    bool passed = true;
    Tick tick = 0;
    std::string description;
};

struct RunArtifacts {
    bool passed = true;
    std::string run_id;
    std::string scenario_id;
    std::string scenario_name;
    std::string profile_id;
    std::uint32_t seed = 0;
    Tick duration_ticks = 0;
    std::string digest;
    std::vector<TelemetryFrame> telemetry;
    std::vector<LogEntry> logs;
    std::vector<StateTransition> transitions;
    std::vector<ScenarioAssertionResult> assertions;
    json summary;
};

class DeviceSimulator {
  public:
    explicit DeviceSimulator(DeviceProfile profile, std::uint32_t seed = 7);

    Tick current_tick() const;
    DeviceState state() const;
    const SensorSample& sensors() const;
    const ActuatorState& actuators() const;
    double target_temperature() const;
    const std::vector<LogEntry>& logs() const;
    const std::vector<StateTransition>& transitions() const;
    std::vector<FaultRecord> active_faults() const;
    bool is_fault_active(FaultType type) const;

    void set_scenario_label(std::string label);
    void apply_command(const Command& command);
    void apply_sensor_override(const SensorOverride& sensor_override);
    void inject_fault(FaultType type, const std::string& reason, bool injected = true);
    void clear_fault(FaultType type, const std::string& reason);
    void add_marker(const std::string& note);
    TelemetryFrame advance_tick();
    json snapshot_json() const;

  private:
    void transition_to(DeviceState next, const std::string& reason);
    void raise_fault(FaultType type, const std::string& reason, bool injected);
    void clear_fault_internal(FaultType type, const std::string& reason);
    void log(Severity severity, EventType event_type, const std::string& message, json context = json::object());
    void evaluate_automatic_faults();
    void update_control_loop();
    void update_sensor_model();
    void normalize_fault_recovery();
    double progress_percentage() const;

    DeviceProfile profile_;
    Tick tick_ = 0;
    DeviceState state_ = DeviceState::Off;
    Tick state_entered_tick_ = 0;
    Tick degraded_since_tick_ = 0;
    Tick last_heartbeat_tick_ = 0;
    std::uint32_t seed_ = 7;
    std::map<FaultType, FaultRecord> faults_;
    SensorSample sensors_;
    ActuatorState actuators_;
    double target_temperature_c_ = 60.0;
    bool cycle_requested_ = false;
    std::string scenario_label_;
    std::vector<LogEntry> logs_;
    std::vector<StateTransition> transitions_;
};

RunArtifacts run_scenario(
    const ScenarioDefinition& scenario,
    const DeviceProfile& profile,
    const std::filesystem::path& output_dir);

RunArtifacts replay_run(
    const std::filesystem::path& replay_manifest_path,
    const std::filesystem::path& output_dir);

void write_run_artifacts(
    const ScenarioDefinition& scenario,
    const DeviceProfile& profile,
    const RunArtifacts& artifacts,
    const std::filesystem::path& output_dir);

} // namespace devicelab
