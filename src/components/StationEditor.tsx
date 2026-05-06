import { useState, useEffect } from "react";
import { Station } from "../App";

interface StationEditorProps {
  station: Station | null;
  onSave: (station: Station) => void;
  onCancel: () => void;
}

const CODECS: Station["codec"][] = [
  "MP3", "AAC", "AACPlus", "Opus", "OggVorbis", "FLAC", "WAV", "MP2",
];

const PROTOCOLS: Station["protocol"][] = ["Icecast2", "Shoutcast"];

export default function StationEditor({
  station,
  onSave,
  onCancel,
}: StationEditorProps) {
  const isEditing = station !== null;

  const [name, setName] = useState("");
  const [host, setHost] = useState("");
  const [port, setPort] = useState(8000);
  const [mount, setMount] = useState("");
  const [password, setPassword] = useState("");
  const [protocol, setProtocol] = useState<Station["protocol"]>("Icecast2");
  const [codec, setCodec] = useState<Station["codec"]>("MP3");
  const [bitrate, setBitrate] = useState(128);

  useEffect(() => {
    if (station) {
      setName(station.name);
      setHost(station.host);
      setPort(station.port);
      setMount(station.mount);
      setPassword(station.password);
      setProtocol(station.protocol);
      setCodec(station.codec);
      setBitrate(station.bitrate);
    } else {
      setName("");
      setHost("");
      setPort(8000);
      setMount("");
      setPassword("");
      setProtocol("Icecast2");
      setCodec("MP3");
      setBitrate(128);
    }
  }, [station]);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    onSave({
      id: station ? station.id : crypto.randomUUID(),
      name,
      host,
      port,
      mount,
      password,
      protocol,
      codec,
      bitrate,
      connected: station?.connected ?? false,
      listeners: station?.listeners ?? 0,
      streamTime: station?.streamTime ?? 0,
    });
  };

  return (
    <div className="fixed inset-0 bg-black/60 flex items-center justify-center z-50">
      <div className="bg-zinc-900 border border-zinc-700 rounded-lg w-[420px] shadow-2xl">
        <div className="px-4 py-3 border-b border-zinc-800 flex items-center justify-between">
          <h3 className="text-xs font-semibold text-zinc-300 uppercase tracking-wider">
            {isEditing ? "Edit Station" : "Add Station"}
          </h3>
          <button
            onClick={onCancel}
            className="text-zinc-500 hover:text-zinc-300 text-sm leading-none"
          >
            ✕
          </button>
        </div>

        <form onSubmit={handleSubmit} className="p-4 space-y-3">
          <div>
            <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
              Name
            </label>
            <input
              type="text"
              value={name}
              onChange={(e) => setName(e.target.value)}
              required
              className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
              placeholder="Station name"
            />
          </div>

          <div className="grid grid-cols-2 gap-3">
            <div>
              <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                Host
              </label>
              <input
                type="text"
                value={host}
                onChange={(e) => setHost(e.target.value)}
                required
                className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
                placeholder="server.example.com"
              />
            </div>
            <div>
              <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                Port
              </label>
              <input
                type="number"
                value={port}
                onChange={(e) => setPort(Number(e.target.value))}
                required
                min={1}
                max={65535}
                className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
              />
            </div>
          </div>

          <div>
            <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
              Mount Point
            </label>
            <input
              type="text"
              value={mount}
              onChange={(e) => setMount(e.target.value)}
              required
              className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
              placeholder="/stream.mp3"
            />
          </div>

          <div>
            <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
              Password
            </label>
            <input
              type="password"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              required
              className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
              placeholder="Source password"
            />
          </div>

          <div className="grid grid-cols-2 gap-3">
            <div>
              <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                Protocol
              </label>
              <select
                value={protocol}
                onChange={(e) =>
                  setProtocol(e.target.value as Station["protocol"])
                }
                className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
              >
                {PROTOCOLS.map((p) => (
                  <option key={p} value={p}>
                    {p === "Icecast2" ? "Icecast2" : "Shoutcast"}
                  </option>
                ))}
              </select>
            </div>
            <div>
              <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
                Codec
              </label>
              <select
                value={codec}
                onChange={(e) =>
                  setCodec(e.target.value as Station["codec"])
                }
                className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
              >
                {CODECS.map((c) => (
                  <option key={c} value={c}>
                    {c === "AACPlus" ? "AAC+" : c}
                  </option>
                ))}
              </select>
            </div>
          </div>

          <div>
            <label className="block text-[10px] text-zinc-500 uppercase tracking-wider mb-1">
              Bitrate (kbps)
            </label>
            <input
              type="number"
              value={bitrate}
              onChange={(e) => setBitrate(Number(e.target.value))}
              min={16}
              max={512}
              step={8}
              className="w-full bg-zinc-800 border border-zinc-700 rounded px-2 py-1.5 text-zinc-200 text-xs focus:outline-none focus:border-emerald-600"
            />
          </div>

          <div className="flex gap-2 pt-2">
            <button
              type="button"
              onClick={onCancel}
              className="flex-1 px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 text-zinc-400 rounded text-xs font-medium transition-colors border border-zinc-700"
            >
              Cancel
            </button>
            <button
              type="submit"
              className="flex-1 px-3 py-1.5 bg-emerald-700 hover:bg-emerald-600 text-white rounded text-xs font-semibold transition-colors"
            >
              {isEditing ? "Save Changes" : "Add Station"}
            </button>
          </div>
        </form>
      </div>
    </div>
  );
}
