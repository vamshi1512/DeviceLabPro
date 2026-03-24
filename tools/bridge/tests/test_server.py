from __future__ import annotations

import json
from pathlib import Path

from fastapi.testclient import TestClient

from tools.bridge.server import create_app


def write_json(path: Path, payload: dict) -> None:
    path.write_text(json.dumps(payload), encoding="utf-8")


def minimal_project(tmp_path: Path) -> Path:
    for name in ("scenarios", "profiles", "runs"):
        (tmp_path / name).mkdir(parents=True, exist_ok=True)

    write_json(
        tmp_path / "scenarios" / "nominal.json",
        {
            "id": "nominal-behavior",
            "name": "Nominal Behavior",
            "description": "Minimal nominal scenario",
            "profile_id": "thermal-controller-v1",
            "duration_ticks": 6,
            "events": [
                {"tick": 0, "type": "command", "command": {"type": "POWER_ON"}},
                {"tick": 2, "type": "note", "note": "boot complete"},
            ],
            "expectations": [{"tick": 2, "kind": "state_equals", "value": "IDLE"}],
        },
    )
    write_json(
        tmp_path / "profiles" / "profile.json",
        {
            "id": "thermal-controller-v1",
            "name": "Thermal Controller Mk1",
            "description": "Test profile",
            "boot_ticks": 2,
            "shutdown_ticks": 1,
            "recovery_ticks": 3,
            "heartbeat_timeout_ticks": 4,
            "degraded_escalation_ticks": 5,
            "ambient_temperature_c": 24.0,
            "initial_temperature_c": 24.0,
            "nominal_pressure_kpa": 101.3,
            "min_pressure_kpa": 96.0,
            "max_pressure_kpa": 116.0,
            "target_temperature_c": 62.0,
            "warning_temperature_c": 71.0,
            "critical_temperature_c": 79.0,
            "recovery_temperature_c": 58.0,
            "heat_gain_per_tick": 3.6,
            "cooling_per_tick": 2.3,
            "pump_flow_nominal_lpm": 15.0,
            "pump_flow_degraded_lpm": 8.0,
            "pressure_drop_per_tick": 1.1,
            "sensor_noise_temperature_c": 0.0,
            "sensor_noise_pressure_kpa": 0.0,
            "nominal_fan_pwm": 55,
            "boosted_fan_pwm": 92,
        },
    )
    (tmp_path / "runs" / "nominal-seed").mkdir(parents=True, exist_ok=True)
    write_json(
        tmp_path / "runs" / "nominal-seed" / "summary.json",
        {
            "run_id": "nominal-seed-1234",
            "passed": True,
            "digest": "abc123",
            "final_state": "OFF",
            "final_faults": [],
            "assertion_pass_count": 1,
            "assertion_fail_count": 0,
            "scenario": {"id": "nominal-behavior", "name": "Nominal Behavior", "description": "Minimal nominal"},
            "profile": {"id": "thermal-controller-v1", "name": "Thermal Controller Mk1"},
            "assertions": [{"tick": 2, "passed": True, "description": "State matched expected value IDLE"}],
        },
    )
    return tmp_path


def test_overview_exposes_catalog_and_history(tmp_path: Path) -> None:
    project_root = minimal_project(tmp_path)
    app = create_app(project_root)

    with TestClient(app) as client:
        response = client.get("/api/overview")

    assert response.status_code == 200
    payload = response.json()
    assert payload["binary_available"] is False
    assert payload["scenarios"][0]["id"] == "nominal-behavior"
    assert payload["profiles"][0]["id"] == "thermal-controller-v1"
    assert payload["history"][0]["artifact_dir"] == "nominal-seed"


def test_run_request_returns_404_without_cli_binary(tmp_path: Path) -> None:
    project_root = minimal_project(tmp_path)
    app = create_app(project_root)

    with TestClient(app) as client:
        response = client.post("/api/runs/nominal-behavior", json={"profile_id": "thermal-controller-v1"})

    assert response.status_code == 404
    assert "DeviceLab CLI binary not found" in response.json()["detail"]
