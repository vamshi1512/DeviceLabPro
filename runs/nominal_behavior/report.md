# DeviceLab Pro Run Report

- Run ID: `nominal-behavior-3bf0c3c9c54d`
- Scenario: `nominal-behavior` (Nominal Behavior)
- Profile: `thermal-controller-v1` (Thermal Controller Mk1)
- Result: PASS
- Deterministic Digest: `3bf0c3c9c54d19ac`
- Final State: `OFF`
- Active Faults: none

## Scenario Summary

Baseline startup, heating cycle, and clean shutdown with deterministic heartbeats.

## Assertions

- [x] tick 2: State matched expected value IDLE
- [x] tick 3: State matched expected value RUNNING
- [x] tick 6: State matched expected value RUNNING
- [x] tick 8: Temperature above threshold
- [x] tick 13: Temperature above threshold
- [x] tick 14: Fault COMMUNICATION_FAILURE absent
- [x] tick 16: State matched expected value IDLE
- [x] tick 18: State matched expected value OFF

## State Transitions

- tick 0: `OFF` -> `BOOTING` (Power on command)
- tick 2: `BOOTING` -> `IDLE` (Boot completed)
- tick 3: `IDLE` -> `RUNNING` (Start cycle command)
- tick 16: `RUNNING` -> `IDLE` (Stop cycle command)
- tick 17: `IDLE` -> `SHUTDOWN` (Power off command)
- tick 18: `SHUTDOWN` -> `OFF` (Shutdown complete)

## Final Snapshot

```json
{
  "active_faults": [],
  "actuators": {
    "alarm_buzzer": false,
    "fan_pwm": 0,
    "heater_enabled": false,
    "pump_enabled": false,
    "valve_position": 0
  },
  "notes": [],
  "progress_pct": 0.0,
  "scenario_label": "Nominal Behavior",
  "sensors": {
    "flow_lpm": 10.600000000000001,
    "link_ok": true,
    "pressure_kpa": 98.8,
    "temperature_c": 61.96315,
    "valid": true
  },
  "state": "OFF",
  "target_temperature_c": 66.0,
  "tick": 19
}
```
