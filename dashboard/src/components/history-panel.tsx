import type { RunHistoryItem, RunSummary } from "../types/api";

export function HistoryPanel({
  latestSummary,
  history,
}: {
  latestSummary: RunSummary | null;
  history: RunHistoryItem[];
}) {
  return (
    <div className="panel p-6">
      <div className="panel-label">Scenario Results</div>
      <h2 className="mt-2 text-xl font-semibold text-white">Run Exports And Replay Trail</h2>

      {latestSummary ? (
        <div className="mt-5 rounded-[24px] border border-white/10 bg-gradient-to-br from-accent-500/12 to-white/5 p-5">
          <div className="flex items-center justify-between gap-4">
            <div>
              <div className="text-lg font-semibold text-white">{latestSummary.scenario.name}</div>
              <div className="mt-1 text-sm text-slate-400">
                {latestSummary.profile.name} · digest {latestSummary.digest.slice(0, 12)}
              </div>
            </div>
            <div
              className={`signal-chip ${
                latestSummary.passed
                  ? "border-signal-ok/30 bg-signal-ok/10 text-signal-ok"
                  : "border-signal-error/30 bg-signal-error/10 text-signal-error"
              }`}
            >
              {latestSummary.passed ? "pass" : "fail"}
            </div>
          </div>
          <div className="mt-4 grid gap-3 md:grid-cols-3">
            <div className="rounded-2xl border border-white/10 bg-black/10 px-4 py-3">
              <div className="panel-label">Assertions</div>
              <div className="mt-2 text-2xl font-semibold text-white">{latestSummary.assertions.length}</div>
            </div>
            <div className="rounded-2xl border border-white/10 bg-black/10 px-4 py-3">
              <div className="panel-label">Passed</div>
              <div className="mt-2 text-2xl font-semibold text-signal-ok">
                {latestSummary.assertion_pass_count}
              </div>
            </div>
            <div className="rounded-2xl border border-white/10 bg-black/10 px-4 py-3">
              <div className="panel-label">Failed</div>
              <div className="mt-2 text-2xl font-semibold text-signal-error">
                {latestSummary.assertion_fail_count}
              </div>
            </div>
          </div>
          <div className="mt-4 grid gap-2">
            {latestSummary.assertions.slice(0, 3).map((assertion) => (
              <div key={`${assertion.tick}-${assertion.description}`} className="text-sm text-slate-300">
                tick {assertion.tick} · {assertion.description}
              </div>
            ))}
            {latestSummary.assertions.length === 0 ? (
              <div className="text-sm text-slate-400">No assertions recorded for the latest run.</div>
            ) : null}
          </div>
        </div>
      ) : null}

      <div className="mt-5 space-y-3">
        {history.slice(0, 6).map((entry) => (
          <div
            key={entry.run_id}
            className="flex items-center justify-between gap-3 rounded-2xl border border-white/10 bg-black/10 px-4 py-3"
          >
            <div>
              <div className="text-sm font-medium text-white">{entry.scenario_name}</div>
              <div className="mt-1 font-mono text-xs text-slate-500">
                {entry.artifact_dir} · {entry.digest.slice(0, 10)}
              </div>
            </div>
            <div
              className={`rounded-full px-3 py-1 text-xs uppercase tracking-[0.16em] ${
                entry.passed ? "bg-signal-ok/10 text-signal-ok" : "bg-signal-error/10 text-signal-error"
              }`}
            >
              {entry.final_state}
            </div>
          </div>
        ))}
        {history.length === 0 ? (
          <div className="rounded-2xl border border-dashed border-white/10 p-6 text-sm text-slate-400">
            No run history available yet.
          </div>
        ) : null}
      </div>
    </div>
  );
}
