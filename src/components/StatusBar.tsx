interface StatusBarProps {
  uptimeSecs: number;
}

export default function StatusBar({ uptimeSecs }: StatusBarProps) {
  const hours = Math.floor(uptimeSecs / 3600);
  const minutes = Math.floor((uptimeSecs % 3600) / 60);
  const seconds = uptimeSecs % 60;

  return (
    <div className="h-6 bg-zinc-900 border-t border-zinc-800 flex items-center px-3 gap-4 flex-shrink-0">
      {/* Uptime */}
      <div className="flex items-center gap-1.5">
        <div className="w-1.5 h-1.5 rounded-full bg-emerald-500" />
        <span className="text-[10px] text-zinc-500">Uptime:</span>
        <span className="text-[10px] text-zinc-300 font-mono tabular-nums">
          {String(hours).padStart(2, "0")}:{String(minutes).padStart(2, "0")}:
          {String(seconds).padStart(2, "0")}
        </span>
      </div>

      {/* Separator */}
      <div className="h-3 w-px bg-zinc-700" />

      {/* Encoder info */}
      <span className="text-[10px] text-zinc-600">
        Encoder: ffmpeg backend (MP3, AAC, AAC+, Opus, Ogg, FLAC, WAV, MP2)
      </span>

      {/* Spacer */}
      <div className="flex-1" />

      {/* Version / Build */}
      <span className="text-[10px] text-zinc-600">v0.1.0 · Tauri + Rust</span>
    </div>
  );
}
