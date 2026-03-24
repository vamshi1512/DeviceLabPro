import type {
  ActuatorState,
  BridgeEvent,
  LogEntry,
  OverviewPayload,
  ProfileMeta,
  RunHistoryItem,
  RunSummary,
  ScenarioMeta,
  SensorSample,
  TelemetryFrame,
  TransitionEntry,
} from "../types/api";

type JsonObject = Record<string, unknown>;
type ActiveRunStatus = NonNullable<OverviewPayload["active_run"]>;

function expectObject(value: unknown, context: string): JsonObject {
  if (!value || typeof value !== "object" || Array.isArray(value)) {
    throw new Error(`Invalid ${context}: expected object`);
  }
  return value as JsonObject;
}

function expectArray(value: unknown, context: string): unknown[] {
  if (!Array.isArray(value)) {
    throw new Error(`Invalid ${context}: expected array`);
  }
  return value;
}

function expectString(value: unknown, context: string): string {
  if (typeof value !== "string") {
    throw new Error(`Invalid ${context}: expected string`);
  }
  return value;
}

function expectNumber(value: unknown, context: string): number {
  if (typeof value !== "number" || Number.isNaN(value)) {
    throw new Error(`Invalid ${context}: expected number`);
  }
  return value;
}

function expectBoolean(value: unknown, context: string): boolean {
  if (typeof value !== "boolean") {
    throw new Error(`Invalid ${context}: expected boolean`);
  }
  return value;
}

function optionalString(value: unknown, fallback = ""): string {
  return typeof value === "string" ? value : fallback;
}

function optionalNumber(value: unknown, fallback = 0): number {
  return typeof value === "number" && !Number.isNaN(value) ? value : fallback;
}

function nullableNumber(value: unknown, context: string): number | null {
  if (value == null) {
    return null;
  }
  return expectNumber(value, context);
}

function stringArray(value: unknown, context: string): string[] {
  return expectArray(value, context).map((item, index) => expectString(item, `${context}[${index}]`));
}

function parseSensorSample(value: unknown, context: string): SensorSample {
  const object = expectObject(value, context);
  return {
    temperature_c: expectNumber(object.temperature_c, `${context}.temperature_c`),
    pressure_kpa: expectNumber(object.pressure_kpa, `${context}.pressure_kpa`),
    flow_lpm: expectNumber(object.flow_lpm, `${context}.flow_lpm`),
    link_ok: expectBoolean(object.link_ok, `${context}.link_ok`),
    valid: expectBoolean(object.valid, `${context}.valid`),
  };
}

function parseActuatorState(value: unknown, context: string): ActuatorState {
  const object = expectObject(value, context);
  return {
    heater_enabled: expectBoolean(object.heater_enabled, `${context}.heater_enabled`),
    pump_enabled: expectBoolean(object.pump_enabled, `${context}.pump_enabled`),
    fan_pwm: expectNumber(object.fan_pwm, `${context}.fan_pwm`),
    valve_position: expectNumber(object.valve_position, `${context}.valve_position`),
    alarm_buzzer: expectBoolean(object.alarm_buzzer, `${context}.alarm_buzzer`),
  };
}

function parseTelemetryFrame(value: unknown, context: string): TelemetryFrame {
  const object = expectObject(value, context);
  return {
    tick: expectNumber(object.tick, `${context}.tick`),
    state: expectString(object.state, `${context}.state`),
    sensors: parseSensorSample(object.sensors, `${context}.sensors`),
    actuators: parseActuatorState(object.actuators, `${context}.actuators`),
    active_faults: stringArray(object.active_faults ?? [], `${context}.active_faults`),
    target_temperature_c: expectNumber(object.target_temperature_c, `${context}.target_temperature_c`),
    progress_pct: expectNumber(object.progress_pct, `${context}.progress_pct`),
    scenario_label: expectString(object.scenario_label, `${context}.scenario_label`),
    notes: stringArray(object.notes ?? [], `${context}.notes`),
  };
}

function parseLogEntry(value: unknown, context: string): LogEntry {
  const object = expectObject(value, context);
  return {
    tick: expectNumber(object.tick, `${context}.tick`),
    severity: expectString(object.severity, `${context}.severity`),
    event_type: expectString(object.event_type, `${context}.event_type`),
    message: expectString(object.message, `${context}.message`),
    context: object.context && typeof object.context === "object" && !Array.isArray(object.context)
      ? (object.context as Record<string, unknown>)
      : {},
  };
}

function parseTransitionEntry(value: unknown, context: string): TransitionEntry {
  const object = expectObject(value, context);
  return {
    tick: expectNumber(object.tick, `${context}.tick`),
    from: expectString(object.from, `${context}.from`),
    to: expectString(object.to, `${context}.to`),
    reason: expectString(object.reason, `${context}.reason`),
  };
}

function parseScenarioMeta(value: unknown, context: string): ScenarioMeta {
  const object = expectObject(value, context);
  return {
    id: expectString(object.id, `${context}.id`),
    name: expectString(object.name, `${context}.name`),
    description: optionalString(object.description),
    profile_id: expectString(object.profile_id, `${context}.profile_id`),
    duration_ticks: expectNumber(object.duration_ticks, `${context}.duration_ticks`),
    ui_rank: optionalNumber(object.ui_rank, 99),
    file_name: expectString(object.file_name, `${context}.file_name`),
  };
}

function parseProfileMeta(value: unknown, context: string): ProfileMeta {
  const object = expectObject(value, context);
  return {
    id: expectString(object.id, `${context}.id`),
    name: expectString(object.name, `${context}.name`),
    description: optionalString(object.description),
    target_temperature_c: nullableNumber(object.target_temperature_c, `${context}.target_temperature_c`),
    critical_temperature_c: nullableNumber(object.critical_temperature_c, `${context}.critical_temperature_c`),
    file_name: expectString(object.file_name, `${context}.file_name`),
  };
}

function parseRunSummary(value: unknown, context: string): RunSummary {
  const object = expectObject(value, context);
  const scenario = expectObject(object.scenario, `${context}.scenario`);
  const profile = expectObject(object.profile, `${context}.profile`);
  const assertions = expectArray(object.assertions ?? [], `${context}.assertions`);
  return {
    run_id: expectString(object.run_id, `${context}.run_id`),
    passed: expectBoolean(object.passed, `${context}.passed`),
    digest: expectString(object.digest, `${context}.digest`),
    final_state: expectString(object.final_state, `${context}.final_state`),
    final_faults: stringArray(object.final_faults ?? [], `${context}.final_faults`),
    assertion_pass_count: optionalNumber(object.assertion_pass_count, 0),
    assertion_fail_count: optionalNumber(object.assertion_fail_count, 0),
    scenario: {
      id: expectString(scenario.id, `${context}.scenario.id`),
      name: expectString(scenario.name, `${context}.scenario.name`),
      description: optionalString(scenario.description),
    },
    profile: {
      id: expectString(profile.id, `${context}.profile.id`),
      name: expectString(profile.name, `${context}.profile.name`),
      description: optionalString(profile.description),
    },
    assertions: assertions.map((item, index) => {
      const assertion = expectObject(item, `${context}.assertions[${index}]`);
      return {
        tick: expectNumber(assertion.tick, `${context}.assertions[${index}].tick`),
        passed: expectBoolean(assertion.passed, `${context}.assertions[${index}].passed`),
        description: expectString(assertion.description, `${context}.assertions[${index}].description`),
      };
    }),
  };
}

function parseRunHistoryItem(value: unknown, context: string): RunHistoryItem {
  const object = expectObject(value, context);
  return {
    run_id: expectString(object.run_id, `${context}.run_id`),
    scenario_id: expectString(object.scenario_id, `${context}.scenario_id`),
    scenario_name: expectString(object.scenario_name, `${context}.scenario_name`),
    passed: expectBoolean(object.passed, `${context}.passed`),
    digest: expectString(object.digest, `${context}.digest`),
    final_state: expectString(object.final_state, `${context}.final_state`),
    artifact_dir: expectString(object.artifact_dir, `${context}.artifact_dir`),
  };
}

export function parseActiveRunStatus(value: unknown, context = "active_run"): ActiveRunStatus {
  const object = expectObject(value, context);
  return {
    scenario_id: expectString(object.scenario_id, `${context}.scenario_id`),
    profile_id: expectString(object.profile_id, `${context}.profile_id`),
    delay_ms: expectNumber(object.delay_ms, `${context}.delay_ms`),
    artifact_dir: expectString(object.artifact_dir, `${context}.artifact_dir`),
    started_epoch_s: expectNumber(object.started_epoch_s, `${context}.started_epoch_s`),
  };
}

export function parseOverviewPayload(value: unknown): OverviewPayload {
  const object = expectObject(value, "overview");
  return {
    binary_available: expectBoolean(object.binary_available, "overview.binary_available"),
    active_run: object.active_run == null ? null : parseActiveRunStatus(object.active_run, "overview.active_run"),
    current_snapshot:
      object.current_snapshot == null ? null : parseTelemetryFrame(object.current_snapshot, "overview.current_snapshot"),
    telemetry_history: expectArray(object.telemetry_history ?? [], "overview.telemetry_history").map((item, index) =>
      parseTelemetryFrame(item, `overview.telemetry_history[${index}]`),
    ),
    recent_logs: expectArray(object.recent_logs ?? [], "overview.recent_logs").map((item, index) =>
      parseLogEntry(item, `overview.recent_logs[${index}]`),
    ),
    recent_transitions: expectArray(object.recent_transitions ?? [], "overview.recent_transitions").map((item, index) =>
      parseTransitionEntry(item, `overview.recent_transitions[${index}]`),
    ),
    latest_summary:
      object.latest_summary == null ? null : parseRunSummary(object.latest_summary, "overview.latest_summary"),
    recent_summaries: expectArray(object.recent_summaries ?? [], "overview.recent_summaries").map((item, index) =>
      parseRunSummary(item, `overview.recent_summaries[${index}]`),
    ),
    scenarios: expectArray(object.scenarios ?? [], "overview.scenarios").map((item, index) =>
      parseScenarioMeta(item, `overview.scenarios[${index}]`),
    ),
    profiles: expectArray(object.profiles ?? [], "overview.profiles").map((item, index) =>
      parseProfileMeta(item, `overview.profiles[${index}]`),
    ),
    history: expectArray(object.history ?? [], "overview.history").map((item, index) =>
      parseRunHistoryItem(item, `overview.history[${index}]`),
    ),
  };
}

export function parseBridgeEvent(value: unknown): BridgeEvent {
  const object = expectObject(value, "bridge_event");
  const type = expectString(object.type, "bridge_event.type");
  switch (type) {
    case "bootstrap":
      return { type, payload: parseOverviewPayload(object.payload) };
    case "telemetry":
      return { type, payload: parseTelemetryFrame(object.payload, "bridge_event.payload") };
    case "log":
      return { type, payload: parseLogEntry(object.payload, "bridge_event.payload") };
    case "transition":
      return { type, payload: parseTransitionEntry(object.payload, "bridge_event.payload") };
    case "summary":
      return { type, payload: parseRunSummary(object.payload, "bridge_event.payload") };
    case "run_started":
      return { type, payload: parseActiveRunStatus(object.payload, "bridge_event.payload") };
    case "run_finished": {
      const payload = expectObject(object.payload, "bridge_event.payload");
      return {
        type,
        payload: {
          status: expectString(payload.status, "bridge_event.payload.status"),
          return_code:
            payload.return_code == null ? undefined : expectNumber(payload.return_code, "bridge_event.payload.return_code"),
        },
      };
    }
    case "stderr": {
      const payload = expectObject(object.payload, "bridge_event.payload");
      return {
        type,
        payload: {
          message: payload.message == null ? undefined : expectString(payload.message, "bridge_event.payload.message"),
        },
      };
    }
    case "heartbeat": {
      const payload = expectObject(object.payload, "bridge_event.payload");
      return {
        type,
        payload: {
          epoch_s: expectNumber(payload.epoch_s, "bridge_event.payload.epoch_s"),
        },
      };
    }
    default:
      throw new Error(`Unsupported bridge event type: ${type}`);
  }
}
