import { parseActiveRunStatus, parseOverviewPayload } from "./parsers";
import type { OverviewPayload } from "../types/api";

const bridgeBase = import.meta.env.VITE_BRIDGE_URL ?? "http://localhost:8000";

async function readError(response: Response) {
  try {
    const payload = (await response.json()) as { detail?: string };
    if (payload.detail) {
      return payload.detail;
    }
  } catch {
    return response.statusText || `HTTP ${response.status}`;
  }
  return response.statusText || `HTTP ${response.status}`;
}

export async function fetchOverview(): Promise<OverviewPayload> {
  const response = await fetch(`${bridgeBase}/api/overview`);
  if (!response.ok) {
    throw new Error(`Failed to load overview: ${await readError(response)}`);
  }
  return parseOverviewPayload(await response.json());
}

export async function startScenarioRun(scenarioId: string, profileId: string) {
  const response = await fetch(`${bridgeBase}/api/runs/${scenarioId}`, {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
    },
    body: JSON.stringify({
      profile_id: profileId,
    }),
  });
  if (!response.ok) {
    throw new Error(`Failed to start run: ${await readError(response)}`);
  }
  return parseActiveRunStatus(await response.json(), "run_response");
}

export function bridgeWebSocketUrl() {
  const http = new URL(bridgeBase);
  http.protocol = http.protocol === "https:" ? "wss:" : "ws:";
  http.pathname = "/ws/telemetry";
  return http.toString();
}
