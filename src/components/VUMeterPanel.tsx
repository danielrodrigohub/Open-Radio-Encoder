import { useMemo } from "react";

interface VUMeterPanelProps {
  inputLevelL: number;
  inputLevelR: number;
  outputLevelL: number;
  outputLevelR: number;
}

/** dB to 0-100% normalized for display (-54dB = 0%, 0dB = 100%) */
function dbToPercent(db: number): number {
  return Math.max(0, Math.min(100, ((db + 54) / 54) * 100));
}

function VUBar({ level, label }: { level: number; label: string }) {
  const pct = useMemo(() => dbToPercent(level), [level]);
  const dbDisplay = level <= -53.9 ? "-∞" : level.toFixed(1);

  // Color gradient zones: green (-54 to -18), yellow (-18 to -6), red (-6 to 0)
  const barColor =
    level > -6
      ? "bg-vu-red"
      : level > -18
        ? "bg-vu-yellow"
        : "bg-vu-green";

  return (
    <div className="flex flex-col items-center gap-1">
      {/* Label */}
      <span className="text-[10px] text-zinc-500 uppercase tracking-wider">
        {label}
      </span>

      {/* VU Bar container */}
      <div className="relative w-7 h-48 bg-zinc-800 rounded-sm overflow-hidden border border-zinc-700">
        {/* Background tick marks */}
        <div className="absolute inset-0 flex flex-col justify-between px-0.5 py-1 pointer-events-none">
          {[0, -6, -12, -18, -24, -30, -42, -54].map((db) => {
            const y = 100 - dbToPercent(db);
            return (
              <div
                key={db}
                className="flex items-center gap-0.5"
                style={{ position: "absolute", bottom: `${y}%`, left: 0, right: 0 }}
              >
                <div className="flex-1 h-px bg-zinc-600" />
                <span className="text-[7px] text-zinc-600">{db}</span>
              </div>
            );
          })}
        </div>

        {/* Level fill */}
        <div
          className={`absolute bottom-0 left-0 right-0 ${barColor} transition-all duration-75 ease-linear opacity-80`}
          style={{ height: `${pct}%` }}
        />

        {/* Peak hold line */}
        <div
          className="absolute left-0 right-0 h-px bg-white/70 transition-all duration-100"
          style={{ bottom: `${pct}%` }}
        />
      </div>

      {/* dB readout */}
      <span className="text-[10px] text-zinc-400 font-mono tabular-nums">
        {dbDisplay}
      </span>
    </div>
  );
}

function VUMeterScale({ label }: { label: string }) {
  return (
    <div className="flex flex-col items-center gap-1">
      <span className="text-[10px] text-zinc-600 uppercase tracking-wider">
        {label}
      </span>
      <div className="relative w-7 h-48">
        {/* Scale markings */}
        {[0, -6, -12, -18, -24, -30, -42, -54].map((db) => {
          const y = dbToPercent(db);
          return (
            <div
              key={db}
              className="absolute left-0 right-0 flex items-center gap-0.5"
              style={{ bottom: `${y}%` }}
            >
              <div className="flex-1 h-px bg-zinc-700" />
            </div>
          );
        })}
      </div>
    </div>
  );
}

export default function VUMeterPanel({
  inputLevelL,
  inputLevelR,
  outputLevelL,
  outputLevelR,
}: VUMeterPanelProps) {
  return (
    <div className="flex-1 flex flex-col">
      {/* Section header */}
      <div className="px-4 pt-3 pb-1">
        <h3 className="text-zinc-400 text-[11px] uppercase tracking-wider font-semibold">
          Audio Levels
        </h3>
      </div>

      {/* VU Meters row */}
      <div className="flex-1 flex items-center justify-center gap-10 px-4">
        {/* Input section */}
        <div className="flex flex-col items-center gap-2">
          <span className="text-[11px] text-zinc-500 uppercase tracking-wider font-semibold mb-1">
            Input
          </span>
          <div className="flex gap-8">
            <VUBar level={inputLevelL} label="L" />
            <VUBar level={inputLevelR} label="R" />
          </div>
        </div>

        {/* Separator + DSP indicator */}
        <div className="flex flex-col items-center gap-1">
          <div className="w-px h-16 bg-zinc-700" />
          <span className="text-[9px] text-zinc-600 uppercase">EQ</span>
          <span className="text-[9px] text-zinc-600 uppercase">Comp</span>
          <span className="text-[9px] text-zinc-600 uppercase">Limit</span>
          <div className="w-px h-16 bg-zinc-700" />
        </div>

        {/* Output section */}
        <div className="flex flex-col items-center gap-2">
          <span className="text-[11px] text-zinc-500 uppercase tracking-wider font-semibold mb-1">
            Output
          </span>
          <div className="flex gap-8">
            <VUBar level={outputLevelL} label="L" />
            <VUBar level={outputLevelR} label="R" />
          </div>
        </div>
      </div>

      {/* Legend */}
      <div className="flex justify-center gap-6 pb-3">
        <div className="flex items-center gap-1.5">
          <div className="w-3 h-3 rounded-sm bg-vu-green" />
          <span className="text-[9px] text-zinc-500">-54 to -18 dB</span>
        </div>
        <div className="flex items-center gap-1.5">
          <div className="w-3 h-3 rounded-sm bg-vu-yellow" />
          <span className="text-[9px] text-zinc-500">-18 to -6 dB</span>
        </div>
        <div className="flex items-center gap-1.5">
          <div className="w-3 h-3 rounded-sm bg-vu-red" />
          <span className="text-[9px] text-zinc-500">-6 to 0 dB</span>
        </div>
      </div>
    </div>
  );
}
