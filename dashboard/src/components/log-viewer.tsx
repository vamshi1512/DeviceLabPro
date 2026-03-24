import type { LogEntry } from "../types/api";

const severityTone: Record<string, string> = {
  DEBUG: "text-slate-400",
  INFO: "text-accent-400",
  WARNING: "text-signal-warn",
  ERROR: "text-signal-error",
  CRITICAL: "text-signal-error",
};

export function LogViewer({ logs }: { logs: LogEntry[] }) {
  return (
    <div className="panel p-6">
      <div className="flex items-center justify-between">
        <div>
          <div className="panel-label">Structured Logs</div>
          <h2 className="mt-2 text-xl font-semibold text-white">Recent Event Stream</h2>
        </div>
        <div className="font-mono text-xs text-slate-400">{logs.length} entries buffered</div>
      </div>

      <div className="mt-5 max-h-[420px] space-y-3 overflow-auto pr-1">
        {logs.slice().reverse().map((entry) => (
          <div
            key={`${entry.tick}-${entry.message}-${entry.event_type}`}
            className="rounded-2xl border border-white/10 bg-black/15 p-4"
          >
            <div className="flex items-center justify-between gap-3">
              <div className={`font-mono text-xs uppercase ${severityTone[entry.severity] ?? "text-slate-400"}`}>
                {entry.severity}
              </div>
              <div className="font-mono text-xs text-slate-500">tick {entry.tick}</div>
            </div>
            <div className="mt-2 text-sm font-medium text-white">{entry.message}</div>
            <div className="mt-2 text-xs uppercase tracking-[0.18em] text-slate-500">{entry.event_type}</div>
          </div>
        ))}
        {logs.length === 0 ? (
          <div className="rounded-2xl border border-dashed border-white/10 p-6 text-sm text-slate-400">
            No logs yet. Start a scenario to populate the stream.
          </div>
        ) : null}
      </div>
    </div>
  );
}
