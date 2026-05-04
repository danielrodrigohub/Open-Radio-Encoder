interface TopToolbarProps {
  inputDevice: string;
  onStartCapture: () => void;
  onStopCapture: () => void;
}

export default function TopToolbar({
  inputDevice,
  onStartCapture,
  onStopCapture,
}: TopToolbarProps) {
  return (
    <div className="h-10 bg-zinc-900 border-b border-zinc-800 flex items-center px-3 gap-3 flex-shrink-0">
      {/* App title */}
      <div className="flex items-center gap-2 mr-4">
        <div className="w-5 h-5 rounded bg-emerald-600 flex items-center justify-center">
          <span className="text-[10px] font-bold text-white">OR</span>
        </div>
        <span className="text-sm font-semibold text-zinc-300 tracking-wide">
          Open Radio Encoder
        </span>
      </div>

      <div className="h-5 w-px bg-zinc-700" />

      {/* Input device selector */}
      <div className="flex items-center gap-2">
        <span className="text-zinc-500 text-[11px] uppercase tracking-wider">
          Input:
        </span>
        <select className="bg-zinc-800 border border-zinc-700 rounded px-2 py-0.5 text-zinc-300 text-xs focus:outline-none focus:border-emerald-600">
          <option>{inputDevice}</option>
        </select>
      </div>

      {/* Capture controls */}
      <div className="flex items-center gap-1.5">
        <button
          onClick={onStartCapture}
          className="px-3 py-1 bg-emerald-700 hover:bg-emerald-600 text-white rounded text-xs font-medium transition-colors"
        >
          Start Capture
        </button>
        <button
          onClick={onStopCapture}
          className="px-3 py-1 bg-zinc-800 hover:bg-zinc-700 text-zinc-300 rounded text-xs font-medium transition-colors border border-zinc-700"
        >
          Stop
        </button>
      </div>

      <div className="h-5 w-px bg-zinc-700" />

      {/* Input level indicator */}
      <div className="flex items-center gap-1.5">
        <div className="w-16 h-2 bg-zinc-800 rounded-full overflow-hidden">
          <div
            className="h-full bg-emerald-500 rounded-full transition-all duration-75"
            style={{ width: "45%" }}
          />
        </div>
        <span className="text-zinc-500 text-[10px]">-12dB</span>
      </div>

      <div className="flex-1" />

      {/* Settings / Help */}
      <button className="text-zinc-500 hover:text-zinc-300 transition-colors text-xs px-2 py-1">
        ⚙ Settings
      </button>
      <button className="text-zinc-500 hover:text-zinc-300 transition-colors text-xs px-2 py-1">
        ? Help
      </button>
    </div>
  );
}
