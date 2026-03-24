import type { ProfileMeta, TelemetryFrame } from "../types/api";

function boolLabel(value: boolean) {
  return value ? "enabled" : "idle";
}

function actuatorTone(value: boolean) {
  return value ? "text-signal-ok" : "text-slate-500";
}

export function FaultPanel({
  snapshot,
  profile,
}: {
  snapshot: TelemetryFrame | null;
  profile: ProfileMeta | null;
}) {
  const activeFaults = snapshot?.active_faults ?? [];
  const actuators = snapshot?.actuators;

  return (
    <div className="panel p-6">
      <div className="flex items-start justify-between gap-4">
        <div>
          <div className="panel-label">Fault Domain</div>
          <h2 className="mt-2 text-xl font-semibold text-white">Faults And Actuators</h2>
        </div>
        <div className="rounded-2xl border border-white/10 bg-white/5 px-3 py-2 text-right">
          <div className="font-mono text-xs text-slate-400">profile</div>
          <div className="text-sm font-medium text-white">{profile?.name ?? "unassigned"}</div>
        </div>
      </div>

      <div className="mt-5 flex flex-wrap gap-2">
        {activeFaults.length > 0 ? (
          activeFaults.map((fault) => (
            <span
              key={fault}
              className="signal-chip border-signal-error/30 bg-signal-error/10 text-signal-error"
            >
              {fault.split("_").join(" ")}
            </span>
          ))
        ) : (
          <span className="signal-chip border-signal-ok/30 bg-signal-ok/10 text-signal-ok">
            no active faults
          </span>
        )}
      </div>

      <div className="mt-6 grid gap-3 sm:grid-cols-2">
        <div className="rounded-2xl border border-white/10 bg-black/10 p-4">
          <div className="panel-label">Heater</div>
          <div className={`mt-2 text-lg font-semibold ${actuatorTone(actuators?.heater_enabled ?? false)}`}>
            {boolLabel(actuators?.heater_enabled ?? false)}
          </div>
        </div>
        <div className="rounded-2xl border border-white/10 bg-black/10 p-4">
          <div className="panel-label">Pump</div>
          <div className={`mt-2 text-lg font-semibold ${actuatorTone(actuators?.pump_enabled ?? false)}`}>
            {boolLabel(actuators?.pump_enabled ?? false)}
          </div>
        </div>
        <div className="rounded-2xl border border-white/10 bg-black/10 p-4">
          <div className="panel-label">Fan PWM</div>
          <div className="mt-2 text-lg font-semibold text-white">{actuators?.fan_pwm ?? 0}%</div>
        </div>
        <div className="rounded-2xl border border-white/10 bg-black/10 p-4">
          <div className="panel-label">Valve Position</div>
          <div className="mt-2 text-lg font-semibold text-white">{actuators?.valve_position ?? 0}%</div>
        </div>
      </div>
    </div>
  );
}
