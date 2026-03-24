const states = ["OFF", "BOOTING", "IDLE", "RUNNING", "DEGRADED", "FAULTED", "RECOVERING", "SHUTDOWN"];

export function StateLadder({ currentState }: { currentState?: string | null }) {
  return (
    <div className="panel p-6">
      <div className="flex items-center justify-between">
        <div>
          <div className="panel-label">State Machine</div>
          <h2 className="mt-2 text-xl font-semibold text-white">Execution Ladder</h2>
        </div>
        <div className="rounded-full border border-white/10 bg-white/5 px-3 py-1 font-mono text-xs text-slate-300">
          {currentState ?? "NO SIGNAL"}
        </div>
      </div>

      <div className="mt-6 space-y-3">
        {states.map((state, index) => {
          const active = state === currentState;
          const isFault = state === "FAULTED";
          const isDegraded = state === "DEGRADED";
          return (
            <div key={state} className="flex items-center gap-4">
              <div
                className={`flex h-11 w-11 items-center justify-center rounded-2xl border text-sm font-semibold ${
                  active
                    ? "border-accent-400/60 bg-accent-500/25 text-white shadow-[0_0_20px_rgba(54,183,255,0.35)]"
                    : "border-white/10 bg-white/5 text-slate-400"
                } ${
                  !active && isFault ? "border-signal-error/30 text-signal-error" : ""
                } ${!active && isDegraded ? "border-signal-warn/30 text-signal-warn" : ""}`}
              >
                {index + 1}
              </div>
              <div className="flex-1 rounded-2xl border border-white/10 bg-black/10 px-4 py-3">
                <div className="font-medium text-white">{state}</div>
                <div className="mt-1 text-xs uppercase tracking-[0.18em] text-slate-500">
                  {active ? "Current state" : "Available transition target"}
                </div>
              </div>
            </div>
          );
        })}
      </div>
    </div>
  );
}
