#include "devicelab/device_profile.hpp"

#include <fstream>
#include <stdexcept>

namespace devicelab {

void validate_device_profile(const DeviceProfile& profile) {
    if (profile.id.empty()) {
        throw std::runtime_error("Device profile id must not be empty");
    }
    if (profile.name.empty()) {
        throw std::runtime_error("Device profile name must not be empty");
    }
    if (profile.boot_ticks == 0 || profile.shutdown_ticks == 0 || profile.recovery_ticks == 0) {
        throw std::runtime_error("Device profile timing ticks must be greater than zero");
    }
    if (profile.heartbeat_timeout_ticks == 0 || profile.degraded_escalation_ticks == 0) {
        throw std::runtime_error("Device profile watchdog windows must be greater than zero");
    }
    if (!(profile.min_pressure_kpa < profile.nominal_pressure_kpa &&
          profile.nominal_pressure_kpa < profile.max_pressure_kpa)) {
        throw std::runtime_error("Device profile pressure bounds are inconsistent");
    }
    if (!(profile.ambient_temperature_c < profile.warning_temperature_c &&
          profile.warning_temperature_c < profile.critical_temperature_c)) {
        throw std::runtime_error("Device profile thermal thresholds are inconsistent");
    }
    if (!(profile.ambient_temperature_c < profile.target_temperature_c &&
          profile.target_temperature_c < profile.critical_temperature_c)) {
        throw std::runtime_error("Device profile target temperature must be between ambient and critical");
    }
    if (!(profile.ambient_temperature_c < profile.recovery_temperature_c &&
          profile.recovery_temperature_c < profile.critical_temperature_c)) {
        throw std::runtime_error("Device profile recovery temperature must be between ambient and critical");
    }
    if (profile.heat_gain_per_tick <= 0.0 || profile.cooling_per_tick <= 0.0) {
        throw std::runtime_error("Device profile thermal rates must be positive");
    }
    if (profile.pump_flow_nominal_lpm <= 0.0 || profile.pump_flow_degraded_lpm <= 0.0) {
        throw std::runtime_error("Device profile flow rates must be positive");
    }
    if (profile.pump_flow_degraded_lpm > profile.pump_flow_nominal_lpm) {
        throw std::runtime_error("Degraded pump flow cannot exceed nominal pump flow");
    }
    if (profile.pressure_drop_per_tick <= 0.0) {
        throw std::runtime_error("Pressure drop rate must be positive");
    }
    if (profile.sensor_noise_temperature_c < 0.0 || profile.sensor_noise_pressure_kpa < 0.0) {
        throw std::runtime_error("Sensor noise values cannot be negative");
    }
    if (profile.nominal_fan_pwm < 0 || profile.nominal_fan_pwm > 100 || profile.boosted_fan_pwm < 0 ||
        profile.boosted_fan_pwm > 100) {
        throw std::runtime_error("Fan PWM values must be between 0 and 100");
    }
}

DeviceProfile device_profile_from_json(const json& j) {
    DeviceProfile profile;
    profile.id = j.value("id", profile.id);
    profile.name = j.value("name", profile.name);
    profile.description = j.value("description", profile.description);
    profile.boot_ticks = j.value("boot_ticks", profile.boot_ticks);
    profile.shutdown_ticks = j.value("shutdown_ticks", profile.shutdown_ticks);
    profile.recovery_ticks = j.value("recovery_ticks", profile.recovery_ticks);
    profile.heartbeat_timeout_ticks = j.value("heartbeat_timeout_ticks", profile.heartbeat_timeout_ticks);
    profile.degraded_escalation_ticks =
        j.value("degraded_escalation_ticks", profile.degraded_escalation_ticks);
    profile.ambient_temperature_c = j.value("ambient_temperature_c", profile.ambient_temperature_c);
    profile.initial_temperature_c = j.value("initial_temperature_c", profile.initial_temperature_c);
    profile.nominal_pressure_kpa = j.value("nominal_pressure_kpa", profile.nominal_pressure_kpa);
    profile.min_pressure_kpa = j.value("min_pressure_kpa", profile.min_pressure_kpa);
    profile.max_pressure_kpa = j.value("max_pressure_kpa", profile.max_pressure_kpa);
    profile.target_temperature_c = j.value("target_temperature_c", profile.target_temperature_c);
    profile.warning_temperature_c = j.value("warning_temperature_c", profile.warning_temperature_c);
    profile.critical_temperature_c = j.value("critical_temperature_c", profile.critical_temperature_c);
    profile.recovery_temperature_c = j.value("recovery_temperature_c", profile.recovery_temperature_c);
    profile.heat_gain_per_tick = j.value("heat_gain_per_tick", profile.heat_gain_per_tick);
    profile.cooling_per_tick = j.value("cooling_per_tick", profile.cooling_per_tick);
    profile.pump_flow_nominal_lpm = j.value("pump_flow_nominal_lpm", profile.pump_flow_nominal_lpm);
    profile.pump_flow_degraded_lpm = j.value("pump_flow_degraded_lpm", profile.pump_flow_degraded_lpm);
    profile.pressure_drop_per_tick = j.value("pressure_drop_per_tick", profile.pressure_drop_per_tick);
    profile.sensor_noise_temperature_c =
        j.value("sensor_noise_temperature_c", profile.sensor_noise_temperature_c);
    profile.sensor_noise_pressure_kpa =
        j.value("sensor_noise_pressure_kpa", profile.sensor_noise_pressure_kpa);
    profile.nominal_fan_pwm = j.value("nominal_fan_pwm", profile.nominal_fan_pwm);
    profile.boosted_fan_pwm = j.value("boosted_fan_pwm", profile.boosted_fan_pwm);
    validate_device_profile(profile);
    return profile;
}

DeviceProfile load_device_profile(const std::filesystem::path& profile_path) {
    std::ifstream input(profile_path);
    if (!input) {
        throw std::runtime_error("Unable to open device profile: " + profile_path.string());
    }

    json j;
    input >> j;
    return device_profile_from_json(j);
}

json device_profile_to_json(const DeviceProfile& profile) {
    return json{
        {"id", profile.id},
        {"name", profile.name},
        {"description", profile.description},
        {"boot_ticks", profile.boot_ticks},
        {"shutdown_ticks", profile.shutdown_ticks},
        {"recovery_ticks", profile.recovery_ticks},
        {"heartbeat_timeout_ticks", profile.heartbeat_timeout_ticks},
        {"degraded_escalation_ticks", profile.degraded_escalation_ticks},
        {"ambient_temperature_c", profile.ambient_temperature_c},
        {"initial_temperature_c", profile.initial_temperature_c},
        {"nominal_pressure_kpa", profile.nominal_pressure_kpa},
        {"min_pressure_kpa", profile.min_pressure_kpa},
        {"max_pressure_kpa", profile.max_pressure_kpa},
        {"target_temperature_c", profile.target_temperature_c},
        {"warning_temperature_c", profile.warning_temperature_c},
        {"critical_temperature_c", profile.critical_temperature_c},
        {"recovery_temperature_c", profile.recovery_temperature_c},
        {"heat_gain_per_tick", profile.heat_gain_per_tick},
        {"cooling_per_tick", profile.cooling_per_tick},
        {"pump_flow_nominal_lpm", profile.pump_flow_nominal_lpm},
        {"pump_flow_degraded_lpm", profile.pump_flow_degraded_lpm},
        {"pressure_drop_per_tick", profile.pressure_drop_per_tick},
        {"sensor_noise_temperature_c", profile.sensor_noise_temperature_c},
        {"sensor_noise_pressure_kpa", profile.sensor_noise_pressure_kpa},
        {"nominal_fan_pwm", profile.nominal_fan_pwm},
        {"boosted_fan_pwm", profile.boosted_fan_pwm},
    };
}

} // namespace devicelab
