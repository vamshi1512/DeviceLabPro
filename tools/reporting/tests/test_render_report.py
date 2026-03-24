from __future__ import annotations

from tools.reporting.render_report import load_jsonl, render_html


def test_load_jsonl_returns_empty_list_for_missing_file(tmp_path) -> None:
    assert load_jsonl(tmp_path / "missing.jsonl") == []


def test_render_html_escapes_dynamic_fields() -> None:
    summary = {
        "scenario": {"name": "<Nominal>", "description": "Validate <b>escaping</b>"},
        "passed": True,
        "run_id": "run-<1>",
        "final_state": "IDLE<script>",
        "digest": "abc123",
        "telemetry_count": 1,
        "log_count": 1,
        "transition_count": 1,
        "assertions": [{"tick": 4, "passed": True, "description": "Reached <idle>"}],
    }
    logs = [
        {"tick": 4, "severity": "INFO", "event_type": "NOTE", "message": "Cooling <stable>"},
    ]
    telemetry = [
        {"tick": 4, "state": "IDLE", "sensors": {"temperature_c": 41.2, "pressure_kpa": 102.4, "flow_lpm": 13.8}},
    ]

    html = render_html(summary, logs, telemetry)

    assert "&lt;Nominal&gt;" in html
    assert "&lt;script&gt;" in html
    assert "Cooling &lt;stable&gt;" in html
