import { useState, useCallback } from "react";

interface DSPSettingsProps {
  onClose: () => void;
  onSetEqBand: (band: number, gainDb: number) => void;
  onSetCompressor: (
    thresholdDb: number,
    ratio: number,
    attackMs: number,
    releaseMs: number,
    makeupGainDb: number
  ) => void;
  onSetLimiter: (ceilingDb: number, releaseMs: number) => void;
}

const EQ_BANDS = [31, 63, 125, 250, 500, 1000, 2000, 4000, 8000, 16000];

function formatFreq(freq: number): string {
  return freq >= 1000 ? `${freq / 1000}K` : `${freq}`;
}

export default function DSPSettings({
  onClose,
  onSetEqBand,
  onSetCompressor,
  onSetLimiter,
}: DSPSettingsProps) {
  const [eqGains, setEqGains] = useState<number[]>(Array(10).fill(0));
  const [thresholdDb, setThresholdDb] = useState(-18);
  const [ratio, setRatio] = useState(4);
  const [attackMs, setAttackMs] = useState(10);
  const [releaseMs, setReleaseMs] = useState(100);
  const [makeupGainDb, setMakeupGainDb] = useState(0);
  const [ceilingDb, setCeilingDb] = useState(-0.3);
  const [limiterReleaseMs, setLimiterReleaseMs] = useState(50);

  const handleEqChange = useCallback(
    (band: number, value: number) => {
      const newGains = [...eqGains];
      newGains[band] = value;
      setEqGains(newGains);
      onSetEqBand(band, value);
    },
    [eqGains, onSetEqBand]
  );

  const handleCompressorApply = useCallback(() => {
    onSetCompressor(thresholdDb, ratio, attackMs, releaseMs, makeupGainDb);
  }, [thresholdDb, ratio, attackMs, releaseMs, makeupGainDb, onSetCompressor]);

  const handleLimiterApply = useCallback(() => {
    onSetLimiter(ceilingDb, limiterReleaseMs);
  }, [ceilingDb, limiterReleaseMs, onSetLimiter]);

  return (
    <div className="fixed inset-0 bg-black/60 flex items-center justify-center z-50">
      <div className="bg-zinc-900 border border-zinc-700 rounded-lg w-[580px] max-h-[80vh] overflow-y-auto shadow-2xl">
        <div className="px-4 py-3 border-b border-zinc-800 flex items-center justify-between sticky top-0 bg-zinc-900">
          <h3 className="text-xs font-semibold text-zinc-300 uppercase tracking-wider">
            DSP Settings
          </h3>
          <button
            onClick={onClose}
            className="text-zinc-500 hover:text-zinc-300 text-sm leading-none"
          >
            ✕
          </button>
        </div>

        <div className="p-4 space-y-5">
          {/* ── Equalizer ── */}
          <div>
            <h4 className="text-[11px] font-semibold text-zinc-400 uppercase tracking-wider mb-3">
              10-Band Equalizer
            </h4>
            <div className="flex items-end gap-1 h-32">
              {EQ_BANDS.map((freq, i) => (
                <div
                  key={freq}
                    className="flex-1 flex flex-col items-center gap-1"
                  >
                    <span className="text-[9px] text-zinc-500 tabular-nums">
                      {eqGains[i] > 0 ? "+" : ""}
                      {eqGains[i].toFixed(1)}
                    </span>
                    <input
                      type="range"
                      min={-15}
                      max={15}
                      step={0.5}
                      value={eqGains[i]}
                      onChange={(e) =>
                        handleEqChange(i, Number(e.target.value))
                      }
                      className="w-full h-20 appearance-none bg-zinc-800 rounded-sm cursor-pointer"
                      style={{
                        writingMode: "vertical-lr",
                        direction: "rtl",
                        accentColor:
                          eqGains[i] > 3
                            ? "#f59e0b"
                            : eqGains[i] < -3
                              ? "#0ea5e9"
                              : "#10b981",
                      }}
                    />
                    <span className="text-[9px] text-zinc-500">
                      {formatFreq(freq)}
                    </span>
                  </div>
                ))}
            </div>
          </div>

          <div className="border-t border-zinc-800" />

          {/* ── Compressor ── */}
          <div>
            <h4 className="text-[11px] font-semibold text-zinc-400 uppercase tracking-wider mb-3">
              Compressor
            </h4>
            <div className="grid grid-cols-2 gap-3">
              <div>
                <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                  Threshold ({thresholdDb} dB)
                </label>
                <input
                  type="range"
                  min={-60}
                  max={0}
                  step={0.5}
                  value={thresholdDb}
                  onChange={(e) => setThresholdDb(Number(e.target.value))}
                  className="w-full accent-emerald-500"
                />
              </div>
              <div>
                <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                  Ratio ({ratio}:1)
                </label>
                <input
                  type="range"
                  min={1}
                  max={20}
                  step={0.5}
                  value={ratio}
                  onChange={(e) => setRatio(Number(e.target.value))}
                  className="w-full accent-emerald-500"
                />
              </div>
              <div>
                <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                  Attack ({attackMs} ms)
                </label>
                <input
                  type="range"
                  min={0.1}
                  max={100}
                  step={0.1}
                  value={attackMs}
                  onChange={(e) => setAttackMs(Number(e.target.value))}
                  className="w-full accent-emerald-500"
                />
              </div>
              <div>
                <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                  Release ({releaseMs} ms)
                </label>
                <input
                  type="range"
                  min={10}
                  max={1000}
                  step={1}
                  value={releaseMs}
                  onChange={(e) => setReleaseMs(Number(e.target.value))}
                  className="w-full accent-emerald-500"
                />
              </div>
              <div className="col-span-2">
                <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                  Makeup Gain ({makeupGainDb} dB)
                </label>
                <input
                  type="range"
                  min={0}
                  max={24}
                  step={0.5}
                  value={makeupGainDb}
                  onChange={(e) => setMakeupGainDb(Number(e.target.value))}
                  className="w-full accent-emerald-500"
                />
              </div>
            </div>
            <button
              onClick={handleCompressorApply}
              className="mt-3 w-full px-3 py-1.5 bg-emerald-700 hover:bg-emerald-600 text-white rounded text-xs font-semibold transition-colors"
            >
              Apply Compressor
            </button>
          </div>

          <div className="border-t border-zinc-800" />

          {/* ── Limiter ── */}
          <div>
            <h4 className="text-[11px] font-semibold text-zinc-400 uppercase tracking-wider mb-3">
              Brickwall Limiter
            </h4>
            <div className="grid grid-cols-2 gap-3">
              <div>
                <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                  Ceiling ({ceilingDb} dB)
                </label>
                <input
                  type="range"
                  min={-6}
                  max={0}
                  step={0.1}
                  value={ceilingDb}
                  onChange={(e) => setCeilingDb(Number(e.target.value))}
                  className="w-full accent-emerald-500"
                />
              </div>
              <div>
                <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                  Release ({limiterReleaseMs} ms)
                </label>
                <input
                  type="range"
                  min={5}
                  max={500}
                  step={1}
                  value={limiterReleaseMs}
                  onChange={(e) => setLimiterReleaseMs(Number(e.target.value))}
                  className="w-full accent-emerald-500"
                />
              </div>
            </div>
            <button
              onClick={handleLimiterApply}
              className="mt-3 w-full px-3 py-1.5 bg-emerald-700 hover:bg-emerald-600 text-white rounded text-xs font-semibold transition-colors"
            >
              Apply Limiter
            </button>
          </div>
        </div>
      </div>
    </div>
  );
}
