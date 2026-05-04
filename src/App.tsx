import { useState, useEffect, useCallback } from "react";
import { listen } from "@tauri-apps/api/event";
import { invoke } from "@tauri-apps/api/core";
import TopToolbar from "./components/TopToolbar";
import StationList from "./components/StationList";
import VUMeterPanel from "./components/VUMeterPanel";
import CurrentSong from "./components/CurrentSong";
import Recording from "./components/Recording";
import StatusBar from "./components/StatusBar";

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

export interface EngineUIState {
  inputLevelLeft: number;
  inputLevelRight: number;
  outputLevelLeft: number;
  outputLevelRight: number;
  currentSong: string;
  recordingSizeMB: number;
  uptimeSecs: number;
  stations: Station[];
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

export default function App() {
  const [stations, setStations] = useState<Station[]>(defaultStations);
  const [inputLevelL, setInputLevelL] = useState(-54);
  const [inputLevelR, setInputLevelR] = useState(-54);
  const [outputLevelL, setOutputLevelL] = useState(-54);
  const [outputLevelR, setOutputLevelR] = useState(-54);
  const [currentSong, setCurrentSong] = useState("No metadata source configured");
  const [recordingSize, setRecordingSize] = useState(0);
  const [uptime, setUptime] = useState(0);
  const [inputDevice, setInputDevice] = useState("Default Input");

  // Listen for engine state updates from Rust backend
  useEffect(() => {
    const unlisten = listen<EngineUIState>("engine-state", (event) => {
      const state = event.payload;
      setInputLevelL(state.inputLevelLeft);
      setInputLevelR(state.inputLevelRight);
      setOutputLevelL(state.outputLevelLeft);
      setOutputLevelR(state.outputLevelRight);
      setCurrentSong(state.currentSong);
      setRecordingSize(state.recordingSizeMB);
      setUptime(state.uptimeSecs);
      setStations(state.stations);
    });

    return () => {
      unlisten.then((fn) => fn());
    };
  }, []);

  const handleConnect = useCallback(async (id: string) => {
    try {
      await invoke("connect_station", { id });
      setStations((prev) =>
        prev.map((s) => (s.id === id ? { ...s, connected: true } : s))
      );
    } catch (e) {
      console.error("Failed to connect station:", e);
    }
  }, []);

  const handleDisconnect = useCallback(async (id: string) => {
    try {
      await invoke("disconnect_station", { id });
      setStations((prev) =>
        prev.map((s) => (s.id === id ? { ...s, connected: false } : s))
      );
    } catch (e) {
      console.error("Failed to disconnect station:", e);
    }
  }, []);

  const handleConnectAll = useCallback(async () => {
    try {
      await invoke("connect_all_stations");
      setStations((prev) => prev.map((s) => ({ ...s, connected: true })));
    } catch (e) {
      console.error("Failed to connect all:", e);
    }
  }, []);

  const handleDisconnectAll = useCallback(async () => {
    try {
      await invoke("disconnect_all_stations");
      setStations((prev) => prev.map((s) => ({ ...s, connected: false })));
    } catch (e) {
      console.error("Failed to disconnect all:", e);
    }
  }, []);

  return (
    <div className="flex flex-col h-screen bg-zinc-950 text-zinc-200 font-mono text-xs select-none">
      <TopToolbar
        inputDevice={inputDevice}
        onStartCapture={() => invoke("start_capture", { deviceName: null })}
        onStopCapture={() => invoke("stop_capture")}
      />
      <div className="flex flex-1 overflow-hidden">
        {/* Left Panel: Stations */}
        <div className="w-72 border-r border-zinc-800 flex flex-col flex-shrink-0">
          <StationList
            stations={stations}
            onConnect={handleConnect}
            onDisconnect={handleDisconnect}
            onConnectAll={handleConnectAll}
            onDisconnectAll={handleDisconnectAll}
          />
        </div>

        {/* Center: VU Meters */}
        <div className="flex-1 flex flex-col">
          <div className="flex-1 flex">
            <VUMeterPanel
              inputLevelL={inputLevelL}
              inputLevelR={inputLevelR}
              outputLevelL={outputLevelL}
              outputLevelR={outputLevelR}
            />
          </div>
          {/* Bottom: Current Song + Recording */}
          <div className="h-32 border-t border-zinc-800 flex">
            <div className="flex-1">
              <CurrentSong song={currentSong} />
            </div>
            <div className="w-72 border-l border-zinc-800">
              <Recording sizeMB={recordingSize} />
            </div>
          </div>
        </div>
      </div>
      <StatusBar uptimeSecs={uptime} />
    </div>
  );
}
