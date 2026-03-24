# DeviceLab Pro Run Report

- Run ID: `failure-recovery-9446ffd282c3`
- Scenario: `failure-recovery` (Failure Recovery)
- Profile: `thermal-controller-v1` (Thermal Controller Mk1)
- Result: PASS
- Deterministic Digest: `9446ffd282c31318`
- Final State: `OFF`
- Active Faults: none

## Scenario Summary

Drives the controller into an overtemperature fault, cools it back into range, acknowledges the fault, and verifies controlled recovery.

## Assertions

- [x] tick 8: State matched expected value FAULTED
- [x] tick 8: Fault OVERHEATING present
- [x] tick 10: Fault OVERHEATING absent
- [x] tick 11: State matched expected value RECOVERING
- [x] tick 14: State matched expected value RUNNING
- [x] tick 18: State matched expected value OFF

## State Transitions

- tick 0: `OFF` -> `BOOTING` (Power on command)
- tick 2: `BOOTING` -> `IDLE` (Boot completed)
- tick 3: `IDLE` -> `RUNNING` (Start cycle command)
- tick 8: `RUNNING` -> `FAULTED` (Critical fault asserted)
- tick 11: `FAULTED` -> `RECOVERING` (Fault acknowledged)
- tick 14: `RECOVERING` -> `RUNNING` (Recovery complete)
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
  "scenario_label": "Failure Recovery",
  "sensors": {
    "flow_lpm": 10.600000000000001,
    "link_ok": true,
    "pressure_kpa": 98.8,
    "temperature_c": 47.4362,
    "valid": true
  },
  "state": "OFF",
  "target_temperature_c": 62.0,
  "tick": 19
}
```
