interface CurrentSongProps {
  song: string;
}

export default function CurrentSong({ song }: CurrentSongProps) {
  return (
    <div className="h-full flex flex-col">
      <div className="px-4 py-2 border-b border-zinc-800">
        <h3 className="text-zinc-400 text-[11px] uppercase tracking-wider font-semibold">
          Current Song
        </h3>
      </div>
      <div className="flex-1 flex items-center px-4">
        <div className="flex items-center gap-3 w-full">
          {/* Equalizer animation bars (decorative) */}
          <div className="flex items-end gap-[2px] h-8">
            {[0.7, 1, 0.5, 0.9, 0.4, 0.8, 0.6, 1, 0.3, 0.7].map((h, i) => (
              <div
                key={i}
                className="w-1 bg-emerald-600/60 rounded-t-sm animate-pulse"
                style={{
                  height: `${h * 100}%`,
                  animationDelay: `${i * 0.1}s`,
                }}
              />
            ))}
          </div>

          {/* Song title + metadata */}
          <div className="flex-1 min-w-0">
            <div className="text-sm text-zinc-200 font-medium truncate">
              {song}
            </div>
            <div className="text-[10px] text-zinc-500 mt-0.5">
              Source: {"<Window Title>"} · Next update: 15s
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
