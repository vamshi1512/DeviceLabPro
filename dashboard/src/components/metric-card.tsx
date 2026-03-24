interface MetricCardProps {
  label: string;
  value: string;
  hint: string;
  accent?: "cyan" | "green" | "amber" | "red";
}

const accentStyles = {
  cyan: "from-accent-500/20 to-accent-400/5 border-accent-500/20",
  green: "from-signal-ok/20 to-signal-ok/5 border-signal-ok/20",
  amber: "from-signal-warn/20 to-signal-warn/5 border-signal-warn/20",
  red: "from-signal-error/20 to-signal-error/5 border-signal-error/20",
};

export function MetricCard({
  label,
  value,
  hint,
  accent = "cyan",
}: MetricCardProps) {
  return (
    <div
      className={`panel relative overflow-hidden border p-5 bg-gradient-to-br ${accentStyles[accent]}`}
    >
      <div className="panel-label">{label}</div>
      <div className="metric-value mt-4">{value}</div>
      <div className="mt-2 text-sm text-slate-400">{hint}</div>
      <div className="absolute right-0 top-0 h-24 w-24 rounded-full bg-white/10 blur-3xl" />
    </div>
  );
}
