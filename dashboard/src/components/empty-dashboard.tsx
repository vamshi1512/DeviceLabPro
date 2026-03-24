import type { RunHistoryItem, ScenarioMeta } from "../types/api";

export function EmptyDashboard({
  scenarios,
  history,
  selectedScenarioId,
  onSelect,
}: {
  scenarios: ScenarioMeta[];
  history: RunHistoryItem[];
  selectedScenarioId: string;
  onSelect: (scenarioId: string, profileId: string) => void;
}) {
  return (
    <div className="panel p-6">
      <div className="flex flex-wrap items-start justify-between gap-4">
        <div>
          <div className="panel-label">Live Session</div>
          <h2 className="mt-2 text-2xl font-semibold text-white">No active telemetry stream</h2>
          <p className="mt-3 max-w-2xl text-sm leading-7 text-slate-400">
            Pick a demo scenario to hydrate the dashboard with a deterministic run. The bridge will stream telemetry,
            logs, transitions, and the final scenario summary from the real CLI.
          </p>
        </div>
        <div className="rounded-[24px] border border-white/10 bg-black/15 px-5 py-4 text-right">
          <div className="panel-label">Exported Runs</div>
          <div className="mt-2 text-3xl font-semibold text-white">{history.length}</div>
          <div className="mt-1 text-sm text-slate-500">seed artifacts available</div>
        </div>
      </div>

      {scenarios.length > 0 ? (
        <div className="mt-6 grid gap-4 lg:grid-cols-3">
          {scenarios.map((scenario) => {
            const active = scenario.id === selectedScenarioId;
            return (
              <button
                key={scenario.id}
                type="button"
                onClick={() => onSelect(scenario.id, scenario.profile_id)}
                className={`rounded-[24px] border p-5 text-left transition ${
                  active
                    ? "border-accent-400/40 bg-accent-500/10 shadow-[0_0_40px_rgba(54,183,255,0.18)]"
                    : "border-white/10 bg-black/10 hover:border-white/20 hover:bg-white/5"
                }`}
              >
                <div className="panel-label">{scenario.file_name}</div>
                <div className="mt-3 text-lg font-semibold text-white">{scenario.name}</div>
                <div className="mt-2 text-sm leading-6 text-slate-400">{scenario.description}</div>
                <div className="mt-4 flex items-center justify-between text-xs uppercase tracking-[0.16em] text-slate-500">
                  <span>{scenario.duration_ticks} ticks</span>
                  <span>{scenario.profile_id}</span>
                </div>
              </button>
            );
          })}
        </div>
      ) : (
        <div className="mt-6 rounded-[24px] border border-dashed border-white/10 bg-black/10 p-8">
          <div className="text-lg font-semibold text-white">No scenarios found</div>
          <p className="mt-2 max-w-2xl text-sm leading-7 text-slate-400">
            Add JSON scenario definitions under <code>scenarios/</code> to populate the live monitor and scenario
            runner catalog.
          </p>
        </div>
      )}
    </div>
  );
}
