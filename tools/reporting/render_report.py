from __future__ import annotations

import argparse
import html
import json
from pathlib import Path


def load_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def load_jsonl(path: Path) -> list[dict]:
    if not path.exists():
        return []
    return [json.loads(line) for line in path.read_text(encoding="utf-8").splitlines() if line.strip()]


def escape(value: object) -> str:
    return html.escape(str(value), quote=True)


def render_html(summary: dict, logs: list[dict], telemetry: list[dict]) -> str:
    assertions = "".join(
        f"<li class='assertion {'pass' if item['passed'] else 'fail'}'>"
        f"<span>tick {escape(item['tick'])}</span><strong>{escape(item['description'])}</strong></li>"
        for item in summary.get("assertions", [])
    )
    log_rows = "".join(
        "<tr>"
        f"<td>{escape(entry['tick'])}</td>"
        f"<td>{escape(entry['severity'])}</td>"
        f"<td>{escape(entry['event_type'])}</td>"
        f"<td>{escape(entry['message'])}</td>"
        "</tr>"
        for entry in logs[-20:]
    )
    telemetry_rows = "".join(
        "<tr>"
        f"<td>{escape(frame['tick'])}</td>"
        f"<td>{escape(frame['state'])}</td>"
        f"<td>{escape(format(frame['sensors']['temperature_c'], '.1f'))}</td>"
        f"<td>{escape(format(frame['sensors']['pressure_kpa'], '.1f'))}</td>"
        f"<td>{escape(format(frame['sensors']['flow_lpm'], '.1f'))}</td>"
        "</tr>"
        for frame in telemetry[-12:]
    )
    return f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>DeviceLab Pro Report</title>
  <style>
    :root {{
      color-scheme: dark;
      --bg: #07111f;
      --panel: rgba(10, 21, 39, 0.82);
      --border: rgba(137, 195, 255, 0.18);
      --text: #edf4ff;
      --muted: #90a6c3;
      --accent: #5db4ff;
      --pass: #4ade80;
      --fail: #f97316;
    }}
    * {{ box-sizing: border-box; }}
    body {{
      margin: 0;
      font-family: "Space Grotesk", "Segoe UI", sans-serif;
      background:
        radial-gradient(circle at top, rgba(93, 180, 255, 0.16), transparent 36%),
        linear-gradient(180deg, #07111f 0%, #030812 100%);
      color: var(--text);
      padding: 32px;
    }}
    .grid {{
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(260px, 1fr));
      gap: 18px;
    }}
    .panel {{
      background: var(--panel);
      border: 1px solid var(--border);
      border-radius: 20px;
      padding: 20px;
      box-shadow: 0 24px 64px rgba(0, 0, 0, 0.28);
      backdrop-filter: blur(16px);
    }}
    h1, h2 {{ margin: 0 0 14px; }}
    .metric {{
      font-size: 32px;
      font-weight: 700;
      margin-top: 10px;
    }}
    .muted {{ color: var(--muted); }}
    .assertion {{
      display: flex;
      justify-content: space-between;
      gap: 14px;
      border-top: 1px solid rgba(255, 255, 255, 0.08);
      padding: 10px 0;
    }}
    .assertion.pass strong {{ color: var(--pass); }}
    .assertion.fail strong {{ color: var(--fail); }}
    table {{
      width: 100%;
      border-collapse: collapse;
      font-size: 14px;
    }}
    th, td {{
      text-align: left;
      padding: 10px 8px;
      border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    }}
  </style>
</head>
<body>
  <div class="grid">
    <section class="panel">
      <h1>{escape(summary["scenario"]["name"])}</h1>
      <div class="muted">{escape(summary["scenario"]["description"])}</div>
      <div class="metric">{escape('PASS' if summary['passed'] else 'FAIL')}</div>
      <div class="muted">run id {escape(summary["run_id"])}</div>
    </section>
    <section class="panel">
      <h2>Final State</h2>
      <div class="metric">{escape(summary["final_state"])}</div>
      <div class="muted">digest {escape(summary["digest"])}</div>
    </section>
    <section class="panel">
      <h2>Counts</h2>
      <div class="muted">telemetry frames {escape(summary.get("telemetry_count", 0))}</div>
      <div class="muted">log entries {escape(summary.get("log_count", 0))}</div>
      <div class="muted">state transitions {escape(summary.get("transition_count", 0))}</div>
    </section>
  </div>

  <section class="panel" style="margin-top: 18px;">
    <h2>Assertions</h2>
    <div>{assertions or '<div class="muted">No assertions recorded.</div>'}</div>
  </section>

  <section class="panel" style="margin-top: 18px;">
    <h2>Recent Telemetry</h2>
    <table>
      <thead>
        <tr><th>Tick</th><th>State</th><th>Temp C</th><th>Pressure kPa</th><th>Flow LPM</th></tr>
      </thead>
      <tbody>{telemetry_rows}</tbody>
    </table>
  </section>

  <section class="panel" style="margin-top: 18px;">
    <h2>Recent Logs</h2>
    <table>
      <thead>
        <tr><th>Tick</th><th>Severity</th><th>Event</th><th>Message</th></tr>
      </thead>
      <tbody>{log_rows}</tbody>
    </table>
  </section>
</body>
</html>"""


def main() -> None:
    parser = argparse.ArgumentParser(description="Render a DeviceLab Pro run summary to standalone HTML.")
    parser.add_argument("summary", type=Path, help="Path to summary.json")
    parser.add_argument("--logs", type=Path, help="Path to logs.jsonl")
    parser.add_argument("--telemetry", type=Path, help="Path to telemetry.jsonl")
    parser.add_argument("--output", type=Path, help="HTML output path")
    args = parser.parse_args()

    summary = load_json(args.summary)
    summary_dir = args.summary.parent
    logs = load_jsonl(args.logs or summary_dir / "logs.jsonl")
    telemetry = load_jsonl(args.telemetry or summary_dir / "telemetry.jsonl")
    output_path = args.output or summary_dir / "report.html"
    output_path.write_text(render_html(summary, logs, telemetry), encoding="utf-8")
    print(output_path)


if __name__ == "__main__":
    main()
