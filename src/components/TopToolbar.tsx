interface DeviceInfo {
  name: string;
  channels: number;
  sample_rate: number;
  is_default: boolean;
}

interface TopToolbarProps {
  devices: DeviceInfo[];
  selectedDevice: string;
  onDeviceChange: (deviceName: string) => void;
  onStartCapture: (deviceName: string | null) => void;
  onStopCapture: () => void;
  isCapturing: boolean;
  inputLevelDb: number;
  onOpenSettings: () => void;
  onOpenMetadata: () => void;
  onOpenHelp: () => void;
}

export default function TopToolbar({
  devices,
  selectedDevice,
  onDeviceChange,
  onStartCapture,
  onStopCapture,
  isCapturing,
  inputLevelDb,
  onOpenSettings,
  onOpenMetadata,
  onOpenHelp,
}: TopToolbarProps) {
  const levelPct = Math.max(0, Math.min(100, ((inputLevelDb + 54) / 54) * 100));
  const dbDisplay =
    inputLevelDb <= -53.9 ? "-∞" : `${Math.round(inputLevelDb)}dB`;
  const barColor =
    inputLevelDb > -6
      ? "bg-red-500"
      : inputLevelDb > -18
        ? "bg-amber-500"
        : "bg-emerald-500";

  return (
    <div className="h-10 bg-zinc-900 border-b border-zinc-800 flex items-center px-3 gap-3 flex-shrink-0">
      <div className="flex items-center gap-2 mr-4">
        <div className="w-5 h-5 rounded bg-emerald-600 flex items-center justify-center">
          <span className="text-[10px] font-bold text-white">OR</span>
        </div>
        <span className="text-sm font-semibold text-zinc-300 tracking-wide">
          Open Radio Encoder
        </span>
      </div>

      <div className="h-5 w-px bg-zinc-700" />

      <div className="flex items-center gap-2">
        <span className="text-zinc-500 text-[11px] uppercase tracking-wider">
          Input:
        </span>
        <select
          value={selectedDevice}
          onChange={(e) => onDeviceChange(e.target.value)}
          className="bg-zinc-800 border border-zinc-700 rounded px-2 py-0.5 text-zinc-300 text-xs focus:outline-none focus:border-emerald-600 max-w-[180px] truncate"
        >
          {devices.length === 0 && (
            <option value="">No devices found</option>
          )}
          {devices.map((d) => (
            <option key={d.name} value={d.name}>
              {d.name} {d.is_default ? "(default)" : ""}
            </option>
          ))}
        </select>
      </div>

      <div className="flex items-center gap-1.5">
        {!isCapturing ? (
          <button
            onClick={() =>
              onStartCapture(selectedDevice || null)
            }
            className="px-3 py-1 bg-emerald-700 hover:bg-emerald-600 text-white rounded text-xs font-medium transition-colors"
          >
            Start Capture
          </button>
        ) : (
          <button
            onClick={onStopCapture}
            className="px-3 py-1 bg-red-700 hover:bg-red-600 text-white rounded text-xs font-medium transition-colors"
          >
            Stop Capture
          </button>
        )}
      </div>

      <div className="h-5 w-px bg-zinc-700" />

      <div className="flex items-center gap-1.5">
        <div className="w-16 h-2 bg-zinc-800 rounded-full overflow-hidden">
          <div
            className={`h-full ${barColor} rounded-full transition-all duration-75`}
            style={{ width: `${levelPct}%` }}
          />
        </div>
        <span className="text-zinc-500 text-[10px] tabular-nums">
          {dbDisplay}
        </span>
      </div>

      <div className="flex-1" />

      <button
        onClick={onOpenMetadata}
        className="text-zinc-500 hover:text-zinc-300 transition-colors text-xs px-2 py-1"
      >
        ♫ Metadata
      </button>
      <button
        onClick={onOpenSettings}
        className="text-zinc-500 hover:text-zinc-300 transition-colors text-xs px-2 py-1"
      >
        ⚙ DSP
      </button>
      <button
        onClick={onOpenHelp}
        className="text-zinc-500 hover:text-zinc-300 transition-colors text-xs px-2 py-1"
      >
        ? Help
      </button>
    </div>
  );
}
