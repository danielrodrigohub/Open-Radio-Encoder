import { save } from "@tauri-apps/plugin-dialog";

interface RecordingProps {
  sizeMB: number;
  isRecording: boolean;
  format: string;
  onStartRecording: (path: string) => void;
  onStopRecording: () => void;
}

export default function Recording({
  sizeMB,
  isRecording,
  format,
  onStartRecording,
  onStopRecording,
}: RecordingProps) {
  const handleClick = async () => {
    if (isRecording) {
      onStopRecording();
    } else {
      const path = await save({
        title: "Save Recording",
        defaultPath: "recording.wav",
        filters: [
          { name: "WAV Audio", extensions: ["wav"] },
          { name: "All Files", extensions: ["*"] },
        ],
      });
      if (path) {
        onStartRecording(path);
      }
    }
  };

  return (
    <div className="h-full flex flex-col">
      <div className="px-3 py-2 border-b border-zinc-800 flex items-center justify-between">
        <h3 className="text-zinc-400 text-[11px] uppercase tracking-wider font-semibold">
          Recording
        </h3>
        {isRecording && (
          <div className="flex items-center gap-1">
            <div className="w-2 h-2 rounded-full bg-red-500 animate-pulse" />
            <span className="text-[10px] text-red-400 font-medium">REC</span>
          </div>
        )}
      </div>
      <div className="flex-1 flex flex-col justify-center px-3 gap-1.5">
        <div className="flex items-center justify-between">
          <span className="text-[10px] text-zinc-500">File size</span>
          <span className="text-xs text-zinc-300 font-mono tabular-nums">
            {sizeMB.toFixed(1)} MB
          </span>
        </div>
        <div className="flex items-center justify-between">
          <span className="text-[10px] text-zinc-500">Format</span>
          <span className="text-[10px] text-zinc-400">{format}</span>
        </div>
        <button
          onClick={handleClick}
          className={`mt-1 w-full px-2 py-1 rounded text-[11px] font-medium transition-colors ${
            isRecording
              ? "bg-red-900/50 hover:bg-red-900/80 text-red-400 border border-red-800/50"
              : "bg-zinc-800 hover:bg-zinc-700 text-zinc-400 border border-zinc-700"
          }`}
        >
          {isRecording ? "Stop Recording" : "Start Recording"}
        </button>
      </div>
    </div>
  );
}
