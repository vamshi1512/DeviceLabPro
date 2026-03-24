export interface SensorSample {
  temperature_c: number;
  pressure_kpa: number;
  flow_lpm: number;
  link_ok: boolean;
  valid: boolean;
}

export interface ActuatorState {
  heater_enabled: boolean;
  pump_enabled: boolean;
  fan_pwm: number;
  valve_position: number;
  alarm_buzzer: boolean;
}

export interface TelemetryFrame {
  tick: number;
  state: string;
  sensors: SensorSample;
  actuators: ActuatorState;
  active_faults: string[];
  target_temperature_c: number;
  progress_pct: number;
  scenario_label: string;
  notes: string[];
}

export interface LogEntry {
  tick: number;
  severity: string;
  event_type: string;
  message: string;
  context: Record<string, unknown>;
}

export interface TransitionEntry {
  tick: number;
  from: string;
  to: string;
  reason: string;
}

export interface ScenarioMeta {
  id: string;
  name: string;
  description: string;
  profile_id: string;
  duration_ticks: number;
  ui_rank: number;
  file_name: string;
}

export interface ProfileMeta {
  id: string;
  name: string;
  description: string;
  target_temperature_c: number | null;
  critical_temperature_c: number | null;
  file_name: string;
}

export interface RunHistoryItem {
  run_id: string;
  scenario_id: string;
  scenario_name: string;
  passed: boolean;
  digest: string;
  final_state: string;
  artifact_dir: string;
}

export interface RunSummary {
  run_id: string;
  passed: boolean;
  digest: string;
  final_state: string;
  final_faults: string[];
  assertion_pass_count: number;
  assertion_fail_count: number;
  scenario: {
    id: string;
    name: string;
    description: string;
  };
  profile: {
    id: string;
    name: string;
    description: string;
  };
  assertions: Array<{
    tick: number;
    passed: boolean;
    description: string;
  }>;
}

export interface OverviewPayload {
  binary_available: boolean;
  active_run: {
    scenario_id: string;
    profile_id: string;
    delay_ms: number;
    artifact_dir: string;
    started_epoch_s: number;
  } | null;
  current_snapshot: TelemetryFrame | null;
  telemetry_history: TelemetryFrame[];
  recent_logs: LogEntry[];
  recent_transitions: TransitionEntry[];
  latest_summary: RunSummary | null;
  recent_summaries: RunSummary[];
  scenarios: ScenarioMeta[];
  profiles: ProfileMeta[];
  history: RunHistoryItem[];
}

export type BridgeEvent =
  | { type: "bootstrap"; payload: OverviewPayload }
  | { type: "telemetry"; payload: TelemetryFrame }
  | { type: "log"; payload: LogEntry }
  | { type: "transition"; payload: TransitionEntry }
  | { type: "summary"; payload: RunSummary }
  | { type: "run_started"; payload: NonNullable<OverviewPayload["active_run"]> }
  | { type: "run_finished"; payload: { status: string; return_code?: number } }
  | { type: "stderr"; payload: { message?: string } }
  | { type: "heartbeat"; payload: { epoch_s: number } };
