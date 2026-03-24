# DeviceLab Pro Run Report

- Run ID: `degraded-behavior-385b16c50c12`
- Scenario: `degraded-behavior` (Degraded Behavior)
- Profile: `thermal-controller-v1` (Thermal Controller Mk1)
- Result: PASS
- Deterministic Digest: `385b16c50c124841`
- Final State: `IDLE`
- Active Faults: none

## Scenario Summary

Injects a communications fault, verifies degraded operation, and confirms recovery back to nominal control.

## Assertions

- [x] tick 2: State matched expected value IDLE
- [x] tick 7: State matched expected value DEGRADED
- [x] tick 8: Fault COMMUNICATION_FAILURE present
- [x] tick 8: Actuator fan_pwm satisfied minimum value
- [x] tick 10: Observed expected log marker
- [x] tick 11: Fault COMMUNICATION_FAILURE absent
- [x] tick 11: State matched expected value RUNNING
- [x] tick 14: State matched expected value IDLE

## State Transitions

- tick 0: `OFF` -> `BOOTING` (Power on command)
- tick 2: `BOOTING` -> `IDLE` (Boot completed)
- tick 3: `IDLE` -> `RUNNING` (Start cycle command)
- tick 7: `RUNNING` -> `DEGRADED` (Soft fault asserted)
- tick 11: `DEGRADED` -> `RUNNING` (Degraded condition cleared)
- tick 14: `RUNNING` -> `IDLE` (Stop cycle command)

## Final Snapshot

```json
{
  "active_faults": [],
  "actuators": {
    "alarm_buzzer": false,
    "fan_pwm": 42,
    "heater_enabled": false,
    "pump_enabled": false,
    "valve_position": 12
  },
  "notes": [],
  "progress_pct": 100.0,
  "scenario_label": "Degraded Behavior",
  "sensors": {
    "flow_lpm": 12.8,
    "link_ok": true,
    "pressure_kpa": 101.3,
    "temperature_c": 53.0878,
    "valid": true
  },
  "state": "IDLE",
  "target_temperature_c": 62.0,
  "tick": 15
}
```
