use parking_lot::Mutex;
use std::sync::Arc;
use tauri::{AppHandle, Emitter, Manager};
use tokio::sync::broadcast;

use crate::audio::capture::AudioCapture;
use crate::audio::encoder::EncoderManager;
use crate::audio::mixer::AudioMixer;
use crate::audio::vu_meter::VuMeter;

pub struct AudioEngine {
    pub handle: AppHandle,
    pub mixer: Arc<Mutex<AudioMixer>>,
    pub encoder_manager: Arc<Mutex<EncoderManager>>,
    pub vu_meter: Arc<Mutex<VuMeter>>,
    pub is_running: Arc<Mutex<bool>>,
    pub capture_generation: Arc<Mutex<u64>>,
    pub state_tx: broadcast::Sender<EngineState>,
    pub metadata_source: Arc<Mutex<MetadataSource>>,
    pub recording_path: Arc<Mutex<Option<String>>>,
    pub recording_active: Arc<Mutex<bool>>,
    pub stations: Arc<Mutex<Vec<StationInfo>>>,
    pub recording_size_bytes: Arc<Mutex<u64>>,
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub enum MetadataSource {
    WindowTitle(String),
    Application(String),
    Network,
    TextFile(String),
    HttpUrl(String),
    None,
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct EngineState {
    pub input_level_left: f32,
    pub input_level_right: f32,
    pub output_level_left: f32,
    pub output_level_right: f32,
    pub stations: Vec<StationInfo>,
    pub current_song: String,
    pub recording_size_mb: f64,
    pub uptime_secs: u64,
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct StationInfo {
    pub id: String,
    pub name: String,
    pub host: String,
    pub port: u16,
    pub mount: String,
    pub password: String,
    pub protocol: StreamProtocol,
    pub codec: AudioCodec,
    pub bitrate: u32,
    pub connected: bool,
    pub listeners: u32,
    pub stream_time_secs: u64,
    pub bytes_sent: u64,
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub enum StreamProtocol {
    Icecast2,
    Shoutcast,
}

#[derive(Debug, Clone, Copy, serde::Serialize, serde::Deserialize, PartialEq)]
pub enum AudioCodec {
    MP3,
    AAC,
    AACPlus,
    Opus,
    OggVorbis,
    FLAC,
    WAV,
    MP2,
}

impl AudioEngine {
    pub fn new(handle: AppHandle) -> anyhow::Result<()> {
        let (state_tx, _) = broadcast::channel::<EngineState>(256);

        let engine = Arc::new(Self {
            handle: handle.clone(),
            mixer: Arc::new(Mutex::new(AudioMixer::new())),
            encoder_manager: Arc::new(Mutex::new(EncoderManager::new())),
            vu_meter: Arc::new(Mutex::new(VuMeter::new())),
            is_running: Arc::new(Mutex::new(false)),
            capture_generation: Arc::new(Mutex::new(0)),
            state_tx,
            metadata_source: Arc::new(Mutex::new(MetadataSource::None)),
            recording_path: Arc::new(Mutex::new(None)),
            recording_active: Arc::new(Mutex::new(false)),
            stations: Arc::new(Mutex::new(Vec::new())),
            recording_size_bytes: Arc::new(Mutex::new(0)),
        });

        // Spawn state emission loop (VU + station info → frontend at ~30fps)
        let engine_clone = engine.clone();
        let handle_clone = handle.clone();
        tauri::async_runtime::spawn(async move {
            let mut rx = engine_clone.state_tx.subscribe();
            loop {
                match rx.recv().await {
                    Ok(state) => {
                        let _ = handle_clone.emit("engine-state", state);
                    }
                    Err(broadcast::error::RecvError::Lagged(n)) => {
                        log::warn!("State broadcast lagged by {} messages", n);
                    }
                    Err(broadcast::error::RecvError::Closed) => break,
                }
            }
        });

        handle.manage(engine.clone());

        log::info!("AudioEngine initialized and managed");
        Ok(())
    }

    pub fn broadcast_state(&self, state: EngineState) {
        let _ = self.state_tx.send(state);
    }

    pub fn build_state(&self) -> EngineState {
        let vu = self.vu_meter.lock();
        let (left, right) = vu.get_levels();
        drop(vu);

        let recording_size = *self.recording_size_bytes.lock() as f64 / (1024.0 * 1024.0);

        let mut stations = self.stations.lock().clone();
        {
            let em = self.encoder_manager.lock();
            for s in stations.iter_mut() {
                if let Some((active, listeners, stream_time)) = em.get_encoder_state(&s.id) {
                    s.connected = active;
                    s.listeners = listeners;
                    s.stream_time_secs = stream_time;
                }
            }
        }

        EngineState {
            input_level_left: left,
            input_level_right: right,
            output_level_left: left,
            output_level_right: right,
            stations,
            current_song: "No metadata source configured".to_string(),
            recording_size_mb: recording_size,
            uptime_secs: 0,
        }
    }

    pub fn add_station(&self, station: StationInfo) -> anyhow::Result<()> {
        self.encoder_manager.lock().add_station(station.clone());
        self.stations.lock().push(station);
        Ok(())
    }

    pub fn update_station(&self, id: &str, updated: StationInfo) -> anyhow::Result<()> {
        let mut stations = self.stations.lock();
        if let Some(s) = stations.iter_mut().find(|s| s.id == id) {
            *s = updated.clone();
        }
        drop(stations);
        self.encoder_manager.lock().remove_station(id);
        self.encoder_manager.lock().add_station(updated);
        Ok(())
    }

    pub fn remove_station(&self, id: &str) -> anyhow::Result<()> {
        self.encoder_manager.lock().remove_station(id);
        self.stations.lock().retain(|s| s.id != id);
        Ok(())
    }
}

/// Spawns audio capture + processing loop in a dedicated OS thread.
/// cpal::Stream is NOT Send, so we create and hold it entirely within this thread.
pub fn spawn_capture_loop(
    engine: Arc<AudioEngine>,
    device_name: Option<String>,
) -> anyhow::Result<()> {
    let mixer = engine.mixer.clone();
    let vu_meter = engine.vu_meter.clone();
    let is_running = engine.is_running.clone();
    let capture_generation = engine.capture_generation.clone();
    let engine_for_thread = engine.clone();

    let generation = {
        let mut gen = capture_generation.lock();
        *gen += 1;
        *gen
    };

    std::thread::spawn(move || {
        // Create capture stream inside this thread (Stream is !Send)
        let capture = match AudioCapture::new(device_name) {
            Ok(c) => c,
            Err(e) => {
                log::error!("Failed to create audio capture: {}", e);
                let gen = capture_generation.lock();
                if *gen == generation {
                    *is_running.lock() = false;
                }
                return;
            }
        };

        log::info!("Capture loop started (gen {})", generation);
        *is_running.lock() = true;
        let mut frame_count: u64 = 0;
        let ch = capture.channels;

        loop {
            if *capture_generation.lock() != generation {
                break;
            }

            match capture.sample_rx.recv_timeout(std::time::Duration::from_millis(50)) {
                Ok(mut buffer) => {
                    frame_count += 1;
                    mixer.lock().process_buffer(&mut buffer, ch as usize);

                    let mut vu = vu_meter.lock();
                    let (l, r) = vu.process(&buffer, ch as usize);
                    drop(vu);

                    let mut s = engine_for_thread.build_state();
                    s.input_level_left = l;
                    s.input_level_right = r;
                    s.output_level_left = l;
                    s.output_level_right = r;
                    s.current_song = format!("frame={} ch={} L={:.1} R={:.1}", frame_count, ch, l, r);
                    engine_for_thread.broadcast_state(s);
                }
                Err(crossbeam_channel::RecvTimeoutError::Timeout) => {
                    let mut vu = vu_meter.lock();
                    let (l, r) = vu.process(&[], ch as usize);
                    drop(vu);
                    let mut s = engine_for_thread.build_state();
                    s.input_level_left = l;
                    s.input_level_right = r;
                    s.output_level_left = l;
                    s.output_level_right = r;
                    s.current_song = format!("timeout L={:.1} R={:.1}", l, r);
                    engine_for_thread.broadcast_state(s);
                }
                Err(crossbeam_channel::RecvTimeoutError::Disconnected) => {
                    log::warn!("Capture channel disconnected, stopping loop");
                    break;
                }
            }
        }

        // Only set is_running to false if we are still the latest generation
        let gen = capture_generation.lock();
        if *gen == generation {
            *is_running.lock() = false;
            log::info!("Capture loop stopped (gen {})", generation);
        } else {
            log::info!("Capture loop superseded (gen {})", generation);
        }
    });

    Ok(())
}
