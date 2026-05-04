use tauri::State;

use crate::audio::capture::AudioCapture;
use crate::audio::capture::DeviceInfo;
use crate::audio::engine::{AudioCodec, AudioEngine, EngineState, MetadataSource, StationInfo, StreamProtocol};

// ── Capture commands ──

#[tauri::command]
pub async fn start_capture(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
    device_name: Option<String>,
) -> Result<(), String> {
    engine.start_capture(device_name).await.map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn stop_capture(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
) -> Result<(), String> {
    engine.stop_capture().await;
    Ok(())
}

#[tauri::command]
pub fn get_input_devices() -> Result<Vec<DeviceInfo>, String> {
    AudioCapture::list_devices().map_err(|e| e.to_string())
}

// ── Station management commands ──

#[tauri::command]
pub async fn connect_station(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
    id: String,
) -> Result<(), String> {
    engine.encoder_manager.lock().connect_station(&id)
        .map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn disconnect_station(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
    id: String,
) -> Result<(), String> {
    engine.encoder_manager.lock().disconnect_station(&id);
    Ok(())
}

#[tauri::command]
pub async fn connect_all_stations(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
) -> Result<(), String> {
    engine.encoder_manager.lock().connect_all()
        .map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn disconnect_all_stations(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
) -> Result<(), String> {
    engine.encoder_manager.lock().disconnect_all();
    Ok(())
}

// ── Metadata commands ──

#[tauri::command]
pub async fn set_metadata_source(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
    source: MetadataSource,
) -> Result<(), String> {
    *engine.metadata_source.lock() = source;
    Ok(())
}

// ── Recording commands ──

#[tauri::command]
pub async fn start_recording(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
    path: String,
) -> Result<(), String> {
    *engine.recording_path.lock() = Some(path);
    *engine.recording_active.lock() = true;
    Ok(())
}

#[tauri::command]
pub async fn stop_recording(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
) -> Result<(), String> {
    *engine.recording_active.lock() = false;
    *engine.recording_path.lock() = None;
    Ok(())
}

// ── Stats command ──

#[tauri::command]
pub async fn get_encoder_stats(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
) -> Result<EngineState, String> {
    // Build a snapshot of current state
    let vu = engine.vu_meter.lock();
    let (left, right) = vu.get_levels();
    drop(vu);

    let recording_size = if *engine.recording_active.lock() {
        // In production, track actual file size
        0.0
    } else {
        0.0
    };

    Ok(EngineState {
        input_level_left: left,
        input_level_right: right,
        output_level_left: left,
        output_level_right: right,
        stations: vec![], // Populate from encoder manager
        current_song: "No metadata source configured".to_string(),
        recording_size_mb: recording_size,
        uptime_secs: 0,
    })
}

// ── DSP commands ──

#[tauri::command]
pub async fn set_eq_band(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
    band: usize,
    gain_db: f32,
) -> Result<(), String> {
    engine.mixer.lock().equalizer.set_gain(band, gain_db);
    Ok(())
}

#[tauri::command]
pub async fn set_compressor(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
    threshold_db: f32,
    ratio: f32,
    attack_ms: f32,
    release_ms: f32,
    makeup_gain_db: f32,
) -> Result<(), String> {
    engine.mixer.lock().compressor.set_params(
        threshold_db, ratio, attack_ms, release_ms, makeup_gain_db,
    );
    Ok(())
}

#[tauri::command]
pub async fn set_limiter(
    engine: State<'_, std::sync::Arc<AudioEngine>>,
    ceiling_db: f32,
    release_ms: f32,
) -> Result<(), String> {
    engine.mixer.lock().limiter.set_params(ceiling_db, release_ms);
    Ok(())
}
