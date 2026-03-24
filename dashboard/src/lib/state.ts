import type { BridgeEvent, OverviewPayload } from "../types/api";

function trimList<T>(items: T[], max: number) {
  return items.slice(-max);
}

export function reduceBridgeEvent(
  current: OverviewPayload | null,
  event: BridgeEvent,
): OverviewPayload | null {
  if (event.type === "bootstrap") {
    return event.payload;
  }
  if (!current) {
    return current;
  }

  switch (event.type) {
    case "telemetry":
      return {
        ...current,
        current_snapshot: event.payload,
        telemetry_history: trimList([...current.telemetry_history, event.payload], 180),
      };
    case "log":
      return {
        ...current,
        recent_logs: trimList([...current.recent_logs, event.payload], 160),
      };
    case "transition":
      return {
        ...current,
        recent_transitions: trimList([...current.recent_transitions, event.payload], 64),
      };
    case "summary":
      return {
        ...current,
        active_run: null,
        latest_summary: event.payload,
        recent_summaries: trimList([event.payload, ...current.recent_summaries], 20),
      };
    case "run_started":
      return {
        ...current,
        active_run: event.payload,
        telemetry_history: [],
        recent_logs: [],
        recent_transitions: [],
      };
    case "run_finished":
      return {
        ...current,
        active_run: null,
      };
    case "stderr":
    case "heartbeat":
      return current;
    default:
      return current;
  }
}
