import type { LogEntry, TransitionEntry } from "../types/api";

export function TimelinePanel({
  transitions,
  logs,
}: {
  transitions: TransitionEntry[];
  logs: LogEntry[];
}) {
  const timeline = [
    ...transitions.map((item) => ({
      tick: item.tick,
      title: `${item.from} -> ${item.to}`,
      detail: item.reason,
      tone: "transition",
    })),
    ...logs
      .filter((item) => item.event_type === "SCENARIO_MARKER" || item.event_type === "FAULT_RAISED")
      .map((item) => ({
        tick: item.tick,
        title: item.event_type.split("_").join(" "),
        detail: item.message,
        tone: item.event_type === "FAULT_RAISED" ? "fault" : "marker",
      })),
  ]
    .sort((left, right) => left.tick - right.tick)
    .slice(-12);

  return (
    <div className="panel p-6">
      <div className="panel-label">Timeline</div>
      <h2 className="mt-2 text-xl font-semibold text-white">Transitions And Markers</h2>

      <div className="mt-6 space-y-4">
        {timeline.map((item) => (
          <div key={`${item.tick}-${item.title}`} className="relative pl-7">
            <div className="absolute left-[9px] top-0 h-full w-px bg-white/10" />
            <div
              className={`absolute left-0 top-1 h-5 w-5 rounded-full border ${
                item.tone === "fault"
                  ? "border-signal-error/40 bg-signal-error/25"
                  : item.tone === "marker"
                    ? "border-signal-warn/40 bg-signal-warn/20"
                    : "border-accent-400/40 bg-accent-400/20"
              }`}
            />
            <div className="rounded-2xl border border-white/10 bg-black/10 p-4">
              <div className="flex items-center justify-between gap-4">
                <div className="text-sm font-medium text-white">{item.title}</div>
                <div className="font-mono text-xs text-slate-500">tick {item.tick}</div>
              </div>
              <div className="mt-2 text-sm text-slate-400">{item.detail}</div>
            </div>
          </div>
        ))}
        {timeline.length === 0 ? (
          <div className="rounded-2xl border border-dashed border-white/10 p-6 text-sm text-slate-400">
            Timeline markers appear when the simulator emits transitions, fault changes, or scenario notes.
          </div>
        ) : null}
      </div>
    </div>
  );
}
