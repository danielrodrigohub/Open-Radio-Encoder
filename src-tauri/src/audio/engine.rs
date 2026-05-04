use parking_lot::Mutex;
use std::sync::Arc;
use tauri::{AppHandle, Emitter};
use tokio::sync::broadcast;

use crate::audio::capture::AudioCapture;
use crate::audio::encoder::EncoderManager;
use crate::audio::mixer::AudioMixer;
use crate::audio::vu_meter::VuMeter;

pub struct AudioEngine {
    pub capture: Arc<Mutex<Option<AudioCapture>>>,
    pub mixer: Arc<Mutex<AudioMixer>>,
    pub encoder_manager: Arc<Mutex<EncoderManager>>,
    pub vu_meter: Arc<Mutex<VuMeter>>,
    pub is_running: Arc<Mutex<bool>>,
    pub state_tx: broadcast::Sender<EngineState>,
    pub metadata_source: Arc<Mutex<MetadataSource>>,
    pub recording_path: Arc<Mutex<Option<String>>>,
    pub recording_active: Arc<Mutex<bool>>,
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
    pub async fn new(handle: AppHandle) -> anyhow::Result<Arc<Self>> {
        let (state_tx, _) = broadcast::channel::<EngineState>(256);

        let engine = Arc::new(Self {
            capture: Arc::new(Mutex::new(None)),
            mixer: Arc::new(Mutex::new(AudioMixer::new())),
            encoder_manager: Arc::new(Mutex::new(EncoderManager::new())),
            vu_meter: Arc::new(Mutex::new(VuMeter::new())),
            is_running: Arc::new(Mutex::new(false)),
            state_tx,
            metadata_source: Arc::new(Mutex::new(MetadataSource::None)),
            recording_path: Arc::new(Mutex::new(None)),
            recording_active: Arc::new(Mutex::new(false)),
        });

        // Spawn the state emission loop (sends VU + station info to frontend at ~30fps)
        let engine_clone = engine.clone();
        let handle_clone = handle.clone();
        tokio::spawn(async move {
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

        log::info!("AudioEngine initialized");
        Ok(engine)
    }

    pub fn broadcast_state(&self, state: EngineState) {
        let _ = self.state_tx.send(state);
    }

    pub async fn start_capture(&self, device_name: Option<String>) -> anyhow::Result<()> {
        let capture = AudioCapture::new(device_name)?;
        let mut cap = self.capture.lock();
        *cap = Some(capture);
        *self.is_running.lock() = true;
        Ok(())
    }

    pub async fn stop_capture(&self) {
        let mut cap = self.capture.lock();
        *cap = None;
        *self.is_running.lock() = false;
    }
}
