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
    pub fn new(handle: AppHandle) -> anyhow::Result<()> {
        let (state_tx, _) = broadcast::channel::<EngineState>(256);

        let engine = Arc::new(Self {
            handle: handle.clone(),
            mixer: Arc::new(Mutex::new(AudioMixer::new())),
            encoder_manager: Arc::new(Mutex::new(EncoderManager::new())),
            vu_meter: Arc::new(Mutex::new(VuMeter::new())),
            is_running: Arc::new(Mutex::new(false)),
            state_tx,
            metadata_source: Arc::new(Mutex::new(MetadataSource::None)),
            recording_path: Arc::new(Mutex::new(None)),
            recording_active: Arc::new(Mutex::new(false)),
        });

        // Spawn state emission loop (VU + station info → frontend at ~30fps)
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

        let recording_size = if *self.recording_active.lock() {
            0.0 // tracked elsewhere
        } else {
            0.0
        };

        EngineState {
            input_level_left: left,
            input_level_right: right,
            output_level_left: left,
            output_level_right: right,
            stations: vec![],
            current_song: "No metadata source configured".to_string(),
            recording_size_mb: recording_size,
            uptime_secs: 0,
        }
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
    let engine_for_thread = engine.clone();

    std::thread::spawn(move || {
        // Create capture stream inside this thread (Stream is !Send)
        let capture = match AudioCapture::new(device_name) {
            Ok(c) => c,
            Err(e) => {
                log::error!("Failed to create audio capture: {}", e);
                *is_running.lock() = false;
                return;
            }
        };

        log::info!("Capture loop started");
        *is_running.lock() = true;

        loop {
            if !*is_running.lock() {
                break;
            }

            // Receive audio buffer from cpal callback
            match capture.sample_rx.recv_timeout(std::time::Duration::from_millis(50)) {
                Ok(mut buffer) => {
                    // Process through DSP chain
                    mixer.lock().process_buffer(&mut buffer, 2);

                    // Update VU meter and broadcast to frontend
                    let mut vu = vu_meter.lock();
                    let (l, r) = vu.process(&buffer, 2);

                    let mut s = engine_for_thread.build_state();
                    s.input_level_left = l;
                    s.input_level_right = r;
                    s.output_level_left = l;
                    s.output_level_right = r;
                    engine_for_thread.broadcast_state(s);
                }
                Err(crossbeam_channel::RecvTimeoutError::Timeout) => {
                    // No data, apply VU meter decay
                    let mut vu = vu_meter.lock();
                    let (l, r) = vu.process(&[], 2);
                    drop(vu);
                    let mut s = engine_for_thread.build_state();
                    s.input_level_left = l;
                    s.input_level_right = r;
                    s.output_level_left = l;
                    s.output_level_right = r;
                    engine_for_thread.broadcast_state(s);
                }
                Err(crossbeam_channel::RecvTimeoutError::Disconnected) => {
                    log::warn!("Capture channel disconnected, stopping loop");
                    break;
                }
            }
        }

        *is_running.lock() = false;
        log::info!("Capture loop stopped");
    });

    Ok(())
}
