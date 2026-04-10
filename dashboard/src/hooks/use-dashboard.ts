import { useEffect, useState, useTransition } from "react";
import { bridgeWebSocketUrl, fetchOverview, startScenarioRun } from "../lib/api";
import { parseBridgeEvent } from "../lib/parsers";
import { reduceBridgeEvent } from "../lib/state";
import type { OverviewPayload, ProfileMeta, ScenarioMeta } from "../types/api";

type ConnectionState = "connecting" | "live" | "offline";
const websocketRetryDelayMs = 1500;

export function useDashboard() {
  const [overview, setOverview] = useState<OverviewPayload | null>(null);
  const [selectedScenarioId, setSelectedScenarioId] = useState("");
  const [selectedProfileId, setSelectedProfileId] = useState("");
  const [selectedDelayMs, setSelectedDelayMs] = useState(400);
  const [connection, setConnection] = useState<ConnectionState>("connecting");
  const [error, setError] = useState<string | null>(null);
  const [isLoading, setIsLoading] = useState(true);
  const [isPending, startTransition] = useTransition();

  async function loadOverview() {
    setIsLoading(true);
    try {
      const payload = await fetchOverview();
      startTransition(() => {
        setOverview(payload);
      });
      setError(null);
      setIsLoading(false);
    } catch (issue) {
      setError(issue instanceof Error ? issue.message : "Failed to load overview");
      setConnection("offline");
      setIsLoading(false);
    }
  }

  useEffect(() => {
    void loadOverview();
  }, []);

  useEffect(() => {
    if (!overview || selectedScenarioId) {
      return;
    }
    const nextScenario = overview.scenarios[0];
    if (!nextScenario) {
      return;
    }
    setSelectedScenarioId(nextScenario.id);
    setSelectedProfileId(nextScenario.profile_id);
  }, [overview, selectedScenarioId]);

  useEffect(() => {
    let disposed = false;
    let retryTimer: ReturnType<typeof setTimeout> | null = null;
    let websocket: WebSocket | null = null;

    const connect = () => {
      if (disposed) {
        return;
      }

      setConnection("connecting");
      websocket = new WebSocket(bridgeWebSocketUrl());

      websocket.onopen = () => {
        setConnection("live");
        setError(null);
      };

      websocket.onmessage = (event) => {
        try {
          const envelope = parseBridgeEvent(JSON.parse(event.data));
          startTransition(() => {
            setOverview((current) => reduceBridgeEvent(current, envelope));
          });

          if (envelope.type === "summary") {
            void loadOverview();
          }
          if (envelope.type === "stderr" && envelope.payload.message) {
            setError(envelope.payload.message);
          }
        } catch (issue) {
          setError(issue instanceof Error ? issue.message : "Received invalid bridge event");
        }
      };

      websocket.onclose = () => {
        if (disposed) {
          return;
        }
        setConnection("offline");
        retryTimer = setTimeout(connect, websocketRetryDelayMs);
      };

      websocket.onerror = () => {
        setConnection("offline");
      };
    };

    connect();

    return () => {
      disposed = true;
      if (retryTimer) {
        clearTimeout(retryTimer);
      }
      websocket?.close();
    };
  }, []);

  async function runSelectedScenario() {
    if (!selectedScenarioId || !selectedProfileId) {
      return;
    }
    try {
      setError(null);
      await startScenarioRun(selectedScenarioId, selectedProfileId, selectedDelayMs);
      await loadOverview();
    } catch (issue) {
      setError(issue instanceof Error ? issue.message : "Failed to start run");
    }
  }

  const selectedScenario =
    overview?.scenarios.find((scenario: ScenarioMeta) => scenario.id === selectedScenarioId) ?? null;
  const selectedProfile =
    overview?.profiles.find((profile: ProfileMeta) => profile.id === selectedProfileId) ?? null;

  return {
    overview,
    selectedScenarioId,
    setSelectedScenarioId,
    selectedProfileId,
    setSelectedProfileId,
    selectedDelayMs,
    setSelectedDelayMs,
    selectedScenario,
    selectedProfile,
    connection,
    error,
    isLoading,
    isPending,
    runSelectedScenario,
    refresh: loadOverview,
  };
}
