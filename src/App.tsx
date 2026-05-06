import { useState, useEffect, useCallback, useRef } from "react";
import { listen } from "@tauri-apps/api/event";
import { invoke } from "@tauri-apps/api/core";
import TopToolbar from "./components/TopToolbar";
import StationList from "./components/StationList";
import VUMeterPanel from "./components/VUMeterPanel";
import CurrentSong from "./components/CurrentSong";
import Recording from "./components/Recording";
import StatusBar from "./components/StatusBar";
import StationEditor from "./components/StationEditor";
import DSPSettings from "./components/DSPSettings";
import MetadataConfig from "./components/MetadataConfig";

export interface Station {
  id: string;
  name: string;
  host: string;
  port: number;
  mount: string;
  password: string;
  protocol: "Icecast2" | "Shoutcast";
  codec: "MP3" | "AAC" | "AACPlus" | "Opus" | "OggVorbis" | "FLAC" | "WAV" | "MP2";
  bitrate: number;
  connected: boolean;
  listeners: number;
  streamTime: number;
}

interface DeviceInfo {
  name: string;
  channels: number;
  sample_rate: number;
  is_default: boolean;
}

interface EngineUIState {
  input_level_left: number;
  input_level_right: number;
  output_level_left: number;
  output_level_right: number;
  stations: Station[];
  current_song: string;
  recording_size_mb: number;
  uptime_secs: number;
}

const defaultStations: Station[] = [
  {
    id: "1",
    name: "Radio Riquelme",
    host: "server1.radio.com",
    port: 8000,
    mount: "/riquelme.mp3",
    password: "********",
    protocol: "Icecast2",
    codec: "MP3",
    bitrate: 128,
    connected: false,
    listeners: 0,
    streamTime: 0,
  },
  {
    id: "2",
    name: "La Cuarta Dimensión",
    host: "server2.radio.com",
    port: 8000,
    mount: "/cuarta.aac",
    password: "********",
    protocol: "Icecast2",
    codec: "AAC",
    bitrate: 96,
    connected: false,
    listeners: 0,
    streamTime: 0,
  },
  {
    id: "3",
    name: "Radio Caracol",
    host: "s3.radio.com",
    port: 8000,
    mount: "/caracol.opus",
    password: "********",
    protocol: "Shoutcast",
    codec: "Opus",
    bitrate: 64,
    connected: false,
    listeners: 0,
    streamTime: 0,
  },
  {
    id: "4",
    name: "La Mega",
    host: "s4.radio.com",
    port: 8000,
    mount: "/mega.mp3",
    password: "********",
    protocol: "Icecast2",
    codec: "MP3",
    bitrate: 192,
    connected: false,
    listeners: 0,
    streamTime: 0,
  },
];

function toEngineStation(s: Station) {
  return {
    id: s.id,
    name: s.name,
    host: s.host,
    port: s.port,
    mount: s.mount,
    password: s.password,
    protocol: s.protocol,
    codec: s.codec,
    bitrate: s.bitrate,
    connected: s.connected,
    listeners: s.listeners,
    stream_time_secs: s.streamTime,
    bytes_sent: 0,
  };
}

export default function App() {
  const [stations, setStations] = useState<Station[]>([]);
  const [inputLevelL, setInputLevelL] = useState(-54);
  const [inputLevelR, setInputLevelR] = useState(-54);
  const [outputLevelL, setOutputLevelL] = useState(-54);
  const [outputLevelR, setOutputLevelR] = useState(-54);
  const [currentSong, setCurrentSong] = useState("No metadata source configured");
  const [recordingSize, setRecordingSize] = useState(0);
  const [uptime, setUptime] = useState(0);
  const [isCapturing, setIsCapturing] = useState(false);
  const [isRecording, setIsRecording] = useState(false);

  const [devices, setDevices] = useState<DeviceInfo[]>([]);
  const [selectedDevice, setSelectedDevice] = useState("");

  const [showStationEditor, setShowStationEditor] = useState(false);
  const [editingStation, setEditingStation] = useState<Station | null>(null);
  const [showDSPSettings, setShowDSPSettings] = useState(false);
  const [showMetadataConfig, setShowMetadataConfig] = useState(false);
  const [showHelp, setShowHelp] = useState(false);

  // ── Seed default stations + load devices on mount ──
  useEffect(() => {
    defaultStations.forEach((s) => {
      invoke("add_station", { station: toEngineStation(s) }).catch(() => {});
    });

    invoke<DeviceInfo[]>("get_input_devices")
      .then((list) => {
        setDevices(list);
        const def = list.find((d) => d.is_default);
        if (def) setSelectedDevice(def.name);
        else if (list.length > 0) setSelectedDevice(list[0].name);
      })
      .catch(console.error);
  }, []);

  // ── Engine state listener ──
  useEffect(() => {
    const unlisten = listen<EngineUIState>("engine-state", (event) => {
      const state = event.payload;
      setInputLevelL(state.input_level_left);
      setInputLevelR(state.input_level_right);
      setOutputLevelL(state.output_level_left);
      setOutputLevelR(state.output_level_right);
      setCurrentSong(state.current_song);
      setRecordingSize(state.recording_size_mb);
      setUptime(state.uptime_secs);
      if (state.stations.length > 0) {
        setStations(
          state.stations.map((s: any) => ({
            id: s.id,
            name: s.name,
            host: s.host,
            port: s.port,
            mount: s.mount,
            password: s.password,
            protocol: s.protocol,
            codec: s.codec,
            bitrate: s.bitrate,
            connected: s.connected,
            listeners: s.listeners,
            streamTime: s.stream_time_secs ?? 0,
          }))
        );
      }
    });

    return () => {
      unlisten.then((fn) => fn());
    };
  }, []);

  // ── Capture ──
  const handleStartCapture = useCallback(
    async (deviceName: string | null) => {
      try {
        await invoke("start_capture", { device_name: deviceName });
        setIsCapturing(true);
      } catch (e) {
        console.error("Capture start failed:", e);
      }
    },
    []
  );

  const handleStopCapture = useCallback(async () => {
    try {
      await invoke("stop_capture");
      setIsCapturing(false);
    } catch (e) {
      console.error("Capture stop failed:", e);
    }
  }, []);

  // Auto-restart capture when input device changes while capturing
  const initialDeviceSet = useRef(false);
  useEffect(() => {
    if (!initialDeviceSet.current) {
      initialDeviceSet.current = true;
      return;
    }
    if (!isCapturing || !selectedDevice) return;
    invoke("stop_capture")
      .then(() =>
        invoke("start_capture", { device_name: selectedDevice })
      )
      .then(() => setIsCapturing(true))
      .catch(console.error);
  }, [selectedDevice]); // eslint-disable-line react-hooks/exhaustive-deps

  // ── Stations ──
  const handleConnect = useCallback(async (id: string) => {
    try {
      await invoke("connect_station", { id });
    } catch (e) {
      console.error("Failed to connect:", e);
    }
  }, []);

  const handleDisconnect = useCallback(async (id: string) => {
    try {
      await invoke("disconnect_station", { id });
    } catch (e) {
      console.error("Failed to disconnect:", e);
    }
  }, []);

  const handleConnectAll = useCallback(async () => {
    try {
      await invoke("connect_all_stations");
    } catch (e) {
      console.error("Failed to connect all:", e);
    }
  }, []);

  const handleDisconnectAll = useCallback(async () => {
    try {
      await invoke("disconnect_all_stations");
    } catch (e) {
      console.error("Failed to disconnect all:", e);
    }
  }, []);

  const handleAddStation = () => {
    setEditingStation(null);
    setShowStationEditor(true);
  };

  const handleEditStation = (station: Station) => {
    setEditingStation(station);
    setShowStationEditor(true);
  };

  const handleSaveStation = async (station: Station) => {
    try {
      if (editingStation) {
        await invoke("update_station", {
          id: station.id,
          station: toEngineStation(station),
        });
      } else {
        await invoke("add_station", { station: toEngineStation(station) });
      }
    } catch (e) {
      console.error("Failed to save station:", e);
    }
    setShowStationEditor(false);
    setEditingStation(null);
  };

  const handleDeleteStation = async (id: string) => {
    try {
      await invoke("remove_station", { id });
      setStations((prev) => prev.filter((s) => s.id !== id));
    } catch (e) {
      console.error("Failed to delete station:", e);
    }
  };

  // ── DSP ──
  const handleSetEqBand = useCallback(
    async (band: number, gainDb: number) => {
      try {
        await invoke("set_eq_band", { band, gain_db: gainDb });
      } catch (e) {
        console.error("Failed to set EQ band:", e);
      }
    },
    []
  );

  const handleSetCompressor = useCallback(
    async (
      thresholdDb: number,
      ratio: number,
      attackMs: number,
      releaseMs: number,
      makeupGainDb: number
    ) => {
      try {
        await invoke("set_compressor", {
          threshold_db: thresholdDb,
          ratio,
          attack_ms: attackMs,
          release_ms: releaseMs,
          makeup_gain_db: makeupGainDb,
        });
      } catch (e) {
        console.error("Failed to set compressor:", e);
      }
    },
    []
  );

  const handleSetLimiter = useCallback(
    async (ceilingDb: number, releaseMs: number) => {
      try {
        await invoke("set_limiter", { ceiling_db: ceilingDb, release_ms: releaseMs });
      } catch (e) {
        console.error("Failed to set limiter:", e);
      }
    },
    []
  );

  // ── Recording ──
  const handleStartRecording = useCallback(async (path: string) => {
    try {
      await invoke("start_recording", { path });
      setIsRecording(true);
    } catch (e) {
      console.error("Failed to start recording:", e);
    }
  }, []);

  const handleStopRecording = useCallback(async () => {
    try {
      await invoke("stop_recording");
      setIsRecording(false);
    } catch (e) {
      console.error("Failed to stop recording:", e);
    }
  }, []);

  // ── Metadata ──
  const handleSetMetadataSource = useCallback(
    async (config: {
      type: string;
      value?: string;
    }) => {
      try {
        let source: any;
        switch (config.type) {
          case "None":
            source = "None";
            break;
          case "WindowTitle":
            source = { WindowTitle: config.value || "" };
            break;
          case "Application":
            source = { Application: config.value || "" };
            break;
          case "Network":
            source = "Network";
            break;
          case "TextFile":
            source = { TextFile: config.value || "" };
            break;
          case "HttpUrl":
            source = { HttpUrl: config.value || "" };
            break;
          default:
            source = "None";
        }
        await invoke("set_metadata_source", { source });
      } catch (e) {
        console.error("Failed to set metadata source:", e);
      }
    },
    []
  );

  return (
    <div className="flex flex-col h-screen bg-zinc-950 text-zinc-200 font-mono text-xs select-none">
      <TopToolbar
        devices={devices}
        selectedDevice={selectedDevice}
        onDeviceChange={setSelectedDevice}
        onStartCapture={handleStartCapture}
        onStopCapture={handleStopCapture}
        isCapturing={isCapturing}
        inputLevelDb={inputLevelL}
        onOpenSettings={() => setShowDSPSettings(true)}
        onOpenMetadata={() => setShowMetadataConfig(true)}
        onOpenHelp={() => setShowHelp(true)}
      />

      <div className="flex flex-1 overflow-hidden">
        <div className="w-72 border-r border-zinc-800 flex flex-col flex-shrink-0">
          <StationList
            stations={stations}
            onConnect={handleConnect}
            onDisconnect={handleDisconnect}
            onConnectAll={handleConnectAll}
            onDisconnectAll={handleDisconnectAll}
            onAdd={handleAddStation}
            onEdit={handleEditStation}
            onDelete={handleDeleteStation}
          />
        </div>

        <div className="flex-1 flex flex-col">
          <div className="flex-1 flex">
            <VUMeterPanel
              inputLevelL={inputLevelL}
              inputLevelR={inputLevelR}
              outputLevelL={outputLevelL}
              outputLevelR={outputLevelR}
            />
          </div>
          <div className="h-32 border-t border-zinc-800 flex">
            <div className="flex-1">
              <CurrentSong song={currentSong} />
            </div>
            <div className="w-72 border-l border-zinc-800">
              <Recording
                sizeMB={recordingSize}
                isRecording={isRecording}
                format="WAV (16-bit PCM)"
                onStartRecording={handleStartRecording}
                onStopRecording={handleStopRecording}
              />
            </div>
          </div>
        </div>
      </div>

      <StatusBar uptimeSecs={uptime} />

      {/* Modals */}
      {showStationEditor && (
        <StationEditor
          station={editingStation}
          onSave={handleSaveStation}
          onCancel={() => {
            setShowStationEditor(false);
            setEditingStation(null);
          }}
        />
      )}

      {showDSPSettings && (
        <DSPSettings
          onClose={() => setShowDSPSettings(false)}
          onSetEqBand={handleSetEqBand}
          onSetCompressor={handleSetCompressor}
          onSetLimiter={handleSetLimiter}
        />
      )}

      {showMetadataConfig && (
        <MetadataConfig
          onClose={() => setShowMetadataConfig(false)}
          onSetSource={handleSetMetadataSource}
        />
      )}

      {showHelp && (
        <div className="fixed inset-0 bg-black/60 flex items-center justify-center z-50">
          <div className="bg-zinc-900 border border-zinc-700 rounded-lg w-[400px] shadow-2xl">
            <div className="px-4 py-3 border-b border-zinc-800 flex items-center justify-between">
              <h3 className="text-xs font-semibold text-zinc-300 uppercase tracking-wider">
                Help / About
              </h3>
              <button
                onClick={() => setShowHelp(false)}
                className="text-zinc-500 hover:text-zinc-300 text-sm leading-none"
              >
                ✕
              </button>
            </div>
            <div className="p-4 space-y-2 text-xs text-zinc-400">
              <p>
                <span className="text-zinc-200 font-semibold">Open Radio Encoder</span>{" "}
                v0.1.0
              </p>
              <p>
                A lightweight multi-encoder for streaming audio to multiple
                Icecast/Shoutcast servers simultaneously.
              </p>
              <div className="border-t border-zinc-800 pt-2 mt-2">
                <p className="text-zinc-500 uppercase text-[10px] mb-1">
                  Getting Started
                </p>
                <ol className="list-decimal list-inside space-y-1 text-[11px]">
                  <li>Select an input device from the toolbar</li>
                  <li>Click <span className="text-emerald-400">Start Capture</span></li>
                  <li>Add stations via the <span className="text-emerald-400">+ Add</span> button</li>
                  <li>Click <span className="text-emerald-400">Connect</span> on each station to start streaming</li>
                  <li>Adjust DSP settings via the ⚙ DSP button as needed</li>
                </ol>
              </div>
              <div className="border-t border-zinc-800 pt-2 mt-2 text-[11px] text-zinc-600">
                Built with Tauri + Rust + React
              </div>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
