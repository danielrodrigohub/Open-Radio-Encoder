import { Station } from "../App";

interface StationListProps {
  stations: Station[];
  onConnect: (id: string) => void;
  onDisconnect: (id: string) => void;
  onConnectAll: () => void;
  onDisconnectAll: () => void;
  onAdd: () => void;
  onEdit: (station: Station) => void;
  onDelete: (id: string) => void;
}

function statusColor(codec: Station["codec"]): string {
  const map: Record<string, string> = {
    MP3: "bg-amber-500",
    AAC: "bg-sky-500",
    AACPlus: "bg-sky-400",
    Opus: "bg-violet-500",
    OggVorbis: "bg-fuchsia-500",
    FLAC: "bg-emerald-500",
    WAV: "bg-blue-500",
    MP2: "bg-orange-500",
  };
  return map[codec] || "bg-zinc-500";
}

export default function StationList({
  stations,
  onConnect,
  onDisconnect,
  onConnectAll,
  onDisconnectAll,
  onAdd,
  onEdit,
  onDelete,
}: StationListProps) {
  const allConnected =
    stations.length > 0 && stations.every((s) => s.connected);

  return (
    <div className="flex flex-col h-full">
      <div className="px-3 py-2 border-b border-zinc-800 flex items-center justify-between">
        <h2 className="text-zinc-400 text-[11px] uppercase tracking-wider font-semibold">
          Stations
        </h2>
        <button
          onClick={onAdd}
          className="text-xs px-2 py-0.5 bg-emerald-700 hover:bg-emerald-600 text-white rounded font-medium transition-colors"
        >
          + Add
        </button>
      </div>

      <div className="flex-1 overflow-y-auto">
        {stations.length === 0 && (
          <div className="px-3 py-6 text-center text-[10px] text-zinc-600">
            No stations configured.
            <br />
            Click &ldquo;+ Add&rdquo; to create one.
          </div>
        )}
        {stations.map((station) => (
          <div
            key={station.id}
            className={`border-b border-zinc-800/50 px-3 py-2.5 hover:bg-zinc-800/30 transition-colors ${
              station.connected ? "bg-zinc-800/20" : ""
            }`}
          >
            <div className="flex items-center gap-2 mb-1.5">
              <div
                className={`w-2 h-2 rounded-full flex-shrink-0 ${
                  station.connected ? "bg-emerald-500" : "bg-zinc-600"
                }`}
              />
              <span className="text-xs font-medium text-zinc-300 truncate flex-1">
                {station.name}
              </span>
              <span
                className={`text-[9px] px-1.5 py-0.5 rounded ${statusColor(
                  station.codec
                )} text-zinc-900 font-semibold`}
              >
                {station.codec === "AACPlus" ? "AAC+" : station.codec}
              </span>
            </div>

            <div className="text-[10px] text-zinc-500 mb-2 space-y-0.5">
              <div>
                {station.host}:{station.port}
                {station.mount}
              </div>
              <div>
                {station.codec} @ {station.bitrate}kbps ·{" "}
                {station.protocol === "Icecast2" ? "Icecast" : "Shoutcast"}
              </div>
              {station.connected && (
                <div className="text-zinc-400">
                  {station.listeners} listeners ·{" "}
                  {formatTime(station.streamTime)}
                </div>
              )}
            </div>

            <div className="flex gap-1">
              <button
                onClick={() =>
                  station.connected
                    ? onDisconnect(station.id)
                    : onConnect(station.id)
                }
                className={`flex-1 px-2 py-1 rounded text-[11px] font-medium transition-colors ${
                  station.connected
                    ? "bg-red-900/50 hover:bg-red-900/80 text-red-400 border border-red-800/50"
                    : "bg-emerald-900/50 hover:bg-emerald-900/80 text-emerald-400 border border-emerald-800/50"
                }`}
              >
                {station.connected ? "Disconnect" : "Connect"}
              </button>
              <button
                onClick={() => onEdit(station)}
                className="px-2 py-1 bg-zinc-800 hover:bg-zinc-700 text-zinc-400 rounded text-[11px] transition-colors border border-zinc-700"
                title="Edit station"
              >
                ✎
              </button>
              <button
                onClick={() => {
                  if (
                    station.connected &&
                    !confirm("Station is connected. Disconnect and delete?")
                  )
                    return;
                  if (confirm(`Delete "${station.name}"?`)) {
                    onDelete(station.id);
                  }
                }}
                className="px-2 py-1 bg-zinc-800 hover:bg-red-900/50 text-zinc-400 hover:text-red-400 rounded text-[11px] transition-colors border border-zinc-700 hover:border-red-800/50"
                title="Delete station"
              >
                ✕
              </button>
            </div>
          </div>
        ))}
      </div>

      {stations.length > 0 && (
        <div className="p-2 border-t border-zinc-800 flex gap-1.5">
          {!allConnected ? (
            <button
              onClick={onConnectAll}
              className="flex-1 px-2 py-1.5 bg-emerald-700 hover:bg-emerald-600 text-white rounded text-[11px] font-semibold transition-colors"
            >
              Connect All
            </button>
          ) : (
            <button
              onClick={onDisconnectAll}
              className="flex-1 px-2 py-1.5 bg-red-700 hover:bg-red-600 text-white rounded text-[11px] font-semibold transition-colors"
            >
              Disconnect All
            </button>
          )}
        </div>
      )}
    </div>
  );
}

function formatTime(seconds: number): string {
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  const s = seconds % 60;
  if (h > 0) return `${h}h ${m}m ${s}s`;
  if (m > 0) return `${m}m ${s}s`;
  return `${s}s`;
}
