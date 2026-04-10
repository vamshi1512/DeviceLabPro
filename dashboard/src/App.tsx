import { Activity, AlertTriangle, Bot, Gauge, Radio, Thermometer, Wifi } from "lucide-react";
import { useDeferredValue } from "react";
import { EmptyDashboard } from "./components/empty-dashboard";
import { FaultPanel } from "./components/fault-panel";
import { HistoryPanel } from "./components/history-panel";
import { LoadingShell } from "./components/loading-shell";
import { LogViewer } from "./components/log-viewer";
import { MetricCard } from "./components/metric-card";
import { StateLadder } from "./components/state-ladder";
import { TelemetryChart } from "./components/telemetry-chart";
import { TimelinePanel } from "./components/timeline-panel";
import { useDashboard } from "./hooks/use-dashboard";

const delayPresets = [
  { value: 80, label: "Fast", hint: "0.08s per tick" },
  { value: 160, label: "Normal", hint: "0.16s per tick" },
  { value: 400, label: "Slow", hint: "0.4s per tick" },
  { value: 800, label: "Demo", hint: "0.8s per tick" },
];

function stateTone(state?: string | null) {
  if (state === "FAULTED") {
    return "border-signal-error/30 bg-signal-error/10 text-signal-error";
  }
  if (state === "DEGRADED") {
    return "border-signal-warn/30 bg-signal-warn/10 text-signal-warn";
  }
  if (state === "RUNNING") {
    return "border-signal-ok/30 bg-signal-ok/10 text-signal-ok";
  }
  return "border-accent-400/30 bg-accent-500/10 text-accent-400";
}

export default function App() {
  const {
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
    refresh,
  } = useDashboard();

  const deferredTelemetry = useDeferredValue(overview?.telemetry_history ?? []);
  const snapshot = overview?.current_snapshot ?? null;
  const hasLiveData = deferredTelemetry.length > 0 || Boolean(snapshot);

  if (isLoading && !overview) {
    return <LoadingShell />;
  }

  if (!overview) {
    return (
      <div className="relative min-h-screen px-4 py-10 md:px-8">
        <div className="mx-auto max-w-3xl panel p-8">
          <div className="panel-label">Bridge Error</div>
          <h1 className="mt-3 text-3xl font-semibold text-white">Dashboard bootstrap failed</h1>
          <p className="mt-4 text-sm leading-7 text-slate-400">{error ?? "Unknown startup error"}</p>
          <button
            className="mt-6 rounded-2xl bg-accent-500 px-5 py-3 text-sm font-semibold text-white"
            type="button"
            onClick={() => void refresh()}
          >
            Retry overview
          </button>
        </div>
      </div>
    );
  }

  const hasScenarioCatalog = overview.scenarios.length > 0;
  const hasProfileCatalog = overview.profiles.length > 0;
  const selectedDelay =
    delayPresets.find((preset) => preset.value === selectedDelayMs) ?? delayPresets[delayPresets.length - 1];
  const activeDelayMs = overview.active_run?.delay_ms ?? selectedDelayMs;

  return (
    <div className="relative min-h-screen overflow-hidden">
      <div className="pointer-events-none absolute inset-0 dashboard-grid bg-dashboard-grid opacity-20" />
      <div className="pointer-events-none absolute inset-x-0 top-0 h-72 bg-gradient-to-b from-accent-500/10 to-transparent blur-3xl" />

      <main className="relative mx-auto max-w-[1600px] px-4 py-6 md:px-8 lg:px-10">
        <section className="panel hero-sheen overflow-hidden px-6 py-6 md:px-8">
          <div className="grid gap-6 xl:grid-cols-[1.2fr,0.9fr]">
            <div className="relative">
              <div className="absolute -left-16 top-0 h-48 w-48 rounded-full bg-accent-500/20 blur-3xl animate-pulseGlow" />
              <div className="panel-label">DeviceLab Pro</div>
              <h1 className="mt-3 max-w-3xl text-4xl font-semibold tracking-tight text-white md:text-5xl">
                Embedded simulation, deterministic replay, and recruiter-ready monitoring in one rig.
              </h1>
              <p className="mt-4 max-w-2xl text-sm leading-7 text-slate-300 md:text-base">
                A portfolio-grade device lab that couples a C++ state machine, structured telemetry, fault injection,
                and scenario automation to a premium dark operations dashboard.
              </p>

              <div className="mt-6 flex flex-wrap items-center gap-3">
                <span className={`signal-chip ${stateTone(snapshot?.state)}`}>
                  <Radio className="h-3.5 w-3.5" />
                  {snapshot?.state ?? "NO DATA"}
                </span>
                <span className="signal-chip border-white/10 bg-white/5 text-slate-300">
                  <Wifi className="h-3.5 w-3.5" />
                  {connection}
                </span>
                <span className="signal-chip border-white/10 bg-white/5 text-slate-300">
                  <Bot className="h-3.5 w-3.5" />
                  {overview?.binary_available ? "CLI online" : "CLI missing"}
                </span>
                <span className="signal-chip border-white/10 bg-white/5 text-slate-300">
                  <Activity className="h-3.5 w-3.5" />
                  {activeDelayMs} ms/tick
                </span>
                {overview.active_run ? (
                  <span className="signal-chip border-accent-400/30 bg-accent-500/10 text-accent-400">
                    active run {overview.active_run.artifact_dir}
                  </span>
                ) : null}
              </div>
            </div>

            <div className="grid gap-4 md:grid-cols-2">
              <div className="rounded-[28px] border border-white/10 bg-black/15 p-5">
                <div className="flex items-center gap-3 text-sm text-slate-300">
                  <Thermometer className="h-4 w-4 text-accent-400" />
                  Thermal loop
                </div>
                <div className="mt-4 text-4xl font-semibold text-white">
                  {snapshot ? `${snapshot.sensors.temperature_c.toFixed(1)}°C` : "--"}
                </div>
                <div className="mt-2 text-sm text-slate-400">
                  target {snapshot ? `${snapshot.target_temperature_c.toFixed(1)}°C` : "--"}
                </div>
                <div className="mt-5 h-2 overflow-hidden rounded-full bg-white/5">
                  <div
                    className="h-full rounded-full bg-gradient-to-r from-accent-500 to-accent-400 transition-all"
                    style={{ width: `${snapshot?.progress_pct ?? 0}%` }}
                  />
                </div>
              </div>

              <div className="rounded-[28px] border border-white/10 bg-black/15 p-5">
                <div className="flex items-center gap-3 text-sm text-slate-300">
                  <Gauge className="h-4 w-4 text-signal-ok" />
                  Control envelope
                </div>
                <div className="mt-4 text-4xl font-semibold text-white">
                  {snapshot ? `${snapshot.sensors.pressure_kpa.toFixed(1)} kPa` : "--"}
                </div>
                <div className="mt-2 text-sm text-slate-400">
                  flow {snapshot ? `${snapshot.sensors.flow_lpm.toFixed(1)} LPM` : "--"}
                </div>
                <div className="mt-5 flex gap-2">
                  {(snapshot?.notes ?? []).map((note) => (
                    <span key={note} className="rounded-full border border-white/10 px-3 py-1 text-xs text-slate-300">
                      {note}
                    </span>
                  ))}
                </div>
              </div>

              <div className="rounded-[28px] border border-white/10 bg-black/15 p-5 md:col-span-2">
                <div className="panel-label">Scenario Console</div>
                <div className="mt-4 grid gap-3 md:grid-cols-2 xl:grid-cols-3">
                  <select
                    className="min-w-0 rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-sm text-white outline-none"
                    value={selectedScenarioId}
                    disabled={!hasScenarioCatalog}
                    onChange={(event) => {
                      const nextId = event.target.value;
                      setSelectedScenarioId(nextId);
                      const matched = overview?.scenarios.find((item) => item.id === nextId);
                      if (matched) {
                        setSelectedProfileId(matched.profile_id);
                      }
                    }}
                  >
                    {overview?.scenarios.map((scenario) => (
                      <option key={scenario.id} value={scenario.id}>
                        {scenario.name}
                      </option>
                    ))}
                  </select>
                  <select
                    className="min-w-0 rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-sm text-white outline-none"
                    value={selectedProfileId}
                    disabled={!hasProfileCatalog}
                    onChange={(event) => setSelectedProfileId(event.target.value)}
                  >
                    {overview?.profiles.map((profile) => (
                      <option key={profile.id} value={profile.id}>
                        {profile.name}
                      </option>
                    ))}
                  </select>
                  <select
                    className="min-w-0 rounded-2xl border border-white/10 bg-white/5 px-4 py-3 text-sm text-white outline-none"
                    value={selectedDelayMs}
                    onChange={(event) => setSelectedDelayMs(Number(event.target.value))}
                  >
                    {delayPresets.map((preset) => (
                      <option key={preset.value} value={preset.value}>
                        {preset.label} · {preset.hint}
                      </option>
                    ))}
                  </select>
                </div>
                <div className="mt-3">
                  <button
                    className="w-full rounded-2xl bg-accent-500 px-5 py-3 text-sm font-semibold text-white transition hover:bg-accent-400 disabled:cursor-not-allowed disabled:opacity-60"
                    type="button"
                    disabled={!overview?.binary_available || isPending || !hasScenarioCatalog || !hasProfileCatalog}
                    onClick={() => void runSelectedScenario()}
                  >
                    {!hasScenarioCatalog
                      ? "No Scenarios"
                      : !overview?.binary_available
                        ? "Build CLI First"
                        : overview?.active_run
                          ? "Streaming..."
                          : "Run Scenario"}
                  </button>
                </div>
                <div className="mt-4 flex flex-wrap items-center justify-between gap-3 text-sm text-slate-400">
                  <div>{selectedScenario?.description ?? "Select a scenario to start a live stream."}</div>
                  <div>{selectedProfile?.description ?? ""}</div>
                </div>
                <div className="mt-3 text-sm text-slate-500">
                  Run pace: <span className="text-slate-300">{selectedDelay.label}</span> at {selectedDelay.hint}.
                  Higher delays make the telemetry easier to watch live.
                </div>
                {!hasScenarioCatalog || !hasProfileCatalog ? (
                  <div className="mt-3 rounded-2xl border border-dashed border-white/10 px-4 py-3 text-sm text-slate-400">
                    Add at least one scenario and one profile JSON document to enable live runs from the dashboard.
                  </div>
                ) : null}
                {error ? (
                  <div className="mt-3 flex items-center justify-between gap-4 rounded-2xl border border-signal-error/20 bg-signal-error/10 px-4 py-3 text-sm text-signal-error">
                    <span>{error}</span>
                    <button type="button" className="text-xs uppercase tracking-[0.18em]" onClick={() => void refresh()}>
                      reload
                    </button>
                  </div>
                ) : null}
              </div>
            </div>
          </div>
        </section>

        <section className="mt-6 grid gap-5 md:grid-cols-2 xl:grid-cols-4">
          <MetricCard
            label="Current Tick"
            value={snapshot ? String(snapshot.tick) : "--"}
            hint="Discrete simulator step"
          />
          <MetricCard
            label="Active Faults"
            value={String(snapshot?.active_faults.length ?? 0)}
            hint="latched or automatic faults"
            accent={(snapshot?.active_faults.length ?? 0) > 0 ? "red" : "green"}
          />
          <MetricCard
            label="Selected Scenario"
            value={selectedScenario ? selectedScenario.name : "--"}
            hint={`${selectedScenario?.duration_ticks ?? 0} ticks scripted`}
            accent="amber"
          />
          <MetricCard
            label="Latest Digest"
            value={overview?.latest_summary?.digest.slice(0, 12) ?? "--"}
            hint="deterministic replay fingerprint"
            accent="cyan"
          />
        </section>

        <section className="mt-6 grid gap-5 xl:grid-cols-12">
          <div className="xl:col-span-8">
            {hasLiveData ? (
              <TelemetryChart data={deferredTelemetry} />
            ) : (
              <EmptyDashboard
                scenarios={overview.scenarios}
                history={overview.history}
                selectedScenarioId={selectedScenarioId}
                onSelect={(scenarioId, profileId) => {
                  setSelectedScenarioId(scenarioId);
                  setSelectedProfileId(profileId);
                }}
              />
            )}
          </div>
          <div className="xl:col-span-4">
            <StateLadder currentState={snapshot?.state ?? null} />
          </div>
        </section>

        <section className="mt-6 grid gap-5 xl:grid-cols-12">
          <div className="xl:col-span-4">
            <FaultPanel snapshot={snapshot} profile={selectedProfile} />
          </div>
          <div className="xl:col-span-4">
            <TimelinePanel transitions={overview?.recent_transitions ?? []} logs={overview?.recent_logs ?? []} />
          </div>
          <div className="xl:col-span-4">
            <HistoryPanel latestSummary={overview?.latest_summary ?? null} history={overview?.history ?? []} />
          </div>
        </section>

        <section className="mt-6 grid gap-5 xl:grid-cols-[1.2fr,0.8fr]">
          <LogViewer logs={overview?.recent_logs ?? []} />
          <div className="panel p-6">
            <div className="panel-label">Ops Notes</div>
            <h2 className="mt-2 text-xl font-semibold text-white">Live Monitoring Readout</h2>
            <div className="mt-6 space-y-4">
              <div className="rounded-2xl border border-white/10 bg-black/10 p-4">
                <div className="flex items-center gap-3 text-sm text-slate-300">
                  <Activity className="h-4 w-4 text-accent-400" />
                  Stream health
                </div>
                <div className="mt-3 text-sm leading-7 text-slate-400">
                  WebSocket envelopes from the bridge update this dashboard in near real time. The same feed carries
                  telemetry frames, logs, transitions, and final scenario summaries.
                </div>
              </div>
              <div className="rounded-2xl border border-white/10 bg-black/10 p-4">
                <div className="flex items-center gap-3 text-sm text-slate-300">
                  <AlertTriangle className="h-4 w-4 text-signal-warn" />
                  Deterministic replay
                </div>
                <div className="mt-3 text-sm leading-7 text-slate-400">
                  Each run exports a replay manifest and digest so CI, local debugging, and the dashboard all refer to
                  the same device behavior trace.
                </div>
              </div>
            </div>
          </div>
        </section>
      </main>
    </div>
  );
}
