#pragma once

#include <filesystem>
#include <string>

#include "devicelab/types.hpp"

namespace devicelab {

struct DeviceProfile {
    std::string id = "thermal-controller-v1";
    std::string name = "Thermal Controller Mk1";
    std::string description = "Baseline embedded thermal control profile";
    Tick boot_ticks = 2;
    Tick shutdown_ticks = 1;
    Tick recovery_ticks = 3;
    Tick heartbeat_timeout_ticks = 4;
    Tick degraded_escalation_ticks = 5;
    double ambient_temperature_c = 24.0;
    double initial_temperature_c = 24.0;
    double nominal_pressure_kpa = 101.3;
    double min_pressure_kpa = 96.0;
    double max_pressure_kpa = 116.0;
    double target_temperature_c = 62.0;
    double warning_temperature_c = 71.0;
    double critical_temperature_c = 79.0;
    double recovery_temperature_c = 58.0;
    double heat_gain_per_tick = 3.6;
    double cooling_per_tick = 2.3;
    double pump_flow_nominal_lpm = 15.0;
    double pump_flow_degraded_lpm = 8.0;
    double pressure_drop_per_tick = 1.1;
    double sensor_noise_temperature_c = 0.0;
    double sensor_noise_pressure_kpa = 0.0;
    int nominal_fan_pwm = 55;
    int boosted_fan_pwm = 92;
};

DeviceProfile load_device_profile(const std::filesystem::path& profile_path);
DeviceProfile device_profile_from_json(const json& j);
void validate_device_profile(const DeviceProfile& profile);
json device_profile_to_json(const DeviceProfile& profile);

} // namespace devicelab
