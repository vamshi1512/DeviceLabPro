import {
  Area,
  AreaChart,
  CartesianGrid,
  Line,
  ResponsiveContainer,
  Tooltip,
  XAxis,
  YAxis,
} from "recharts";
import type { TelemetryFrame } from "../types/api";

export function TelemetryChart({ data }: { data: TelemetryFrame[] }) {
  const chartData = data.map((frame) => ({
    tick: frame.tick,
    temperature: frame.sensors.temperature_c,
    pressure: frame.sensors.pressure_kpa,
    flow: frame.sensors.flow_lpm,
  }));

  return (
    <div className="panel p-6">
      <div className="flex flex-wrap items-center justify-between gap-4">
        <div>
          <div className="panel-label">Live Telemetry</div>
          <h2 className="mt-2 text-xl font-semibold text-white">Thermal And Pressure Envelope</h2>
        </div>
        <div className="flex items-center gap-3 text-xs uppercase tracking-[0.18em] text-slate-400">
          <span className="flex items-center gap-2">
            <span className="h-2.5 w-2.5 rounded-full bg-accent-400" />
            temperature
          </span>
          <span className="flex items-center gap-2">
            <span className="h-2.5 w-2.5 rounded-full bg-signal-ok" />
            pressure
          </span>
        </div>
      </div>

      <div className="mt-6 h-[320px]">
        <ResponsiveContainer width="100%" height="100%">
          <AreaChart data={chartData}>
            <defs>
              <linearGradient id="temperatureFill" x1="0" y1="0" x2="0" y2="1">
                <stop offset="5%" stopColor="#36b7ff" stopOpacity={0.55} />
                <stop offset="95%" stopColor="#36b7ff" stopOpacity={0.02} />
              </linearGradient>
            </defs>
            <CartesianGrid stroke="rgba(255,255,255,0.08)" vertical={false} />
            <XAxis dataKey="tick" stroke="#6b7c98" tickLine={false} axisLine={false} />
            <YAxis stroke="#6b7c98" tickLine={false} axisLine={false} width={42} />
            <Tooltip
              contentStyle={{
                borderRadius: 18,
                border: "1px solid rgba(255,255,255,0.12)",
                backgroundColor: "rgba(9, 17, 31, 0.96)",
                color: "#edf4ff",
              }}
            />
            <Area
              type="monotone"
              dataKey="temperature"
              stroke="#68d1ff"
              strokeWidth={2.5}
              fill="url(#temperatureFill)"
            />
            <Line type="monotone" dataKey="pressure" stroke="#45d483" strokeWidth={2} dot={false} />
            <Line type="monotone" dataKey="flow" stroke="#f7b955" strokeWidth={1.6} dot={false} />
          </AreaChart>
        </ResponsiveContainer>
      </div>
    </div>
  );
}
