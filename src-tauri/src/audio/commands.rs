use std::sync::Arc;
use tauri::{AppHandle, Manager};

use crate::audio::capture::AudioCapture;
use crate::audio::engine::{spawn_capture_loop, AudioEngine, EngineState, MetadataSource, StationInfo};

fn get_engine(handle: &AppHandle) -> Arc<AudioEngine> {
    handle.state::<Arc<AudioEngine>>().inner().clone()
}

// ── Capture commands ──

#[tauri::command]
pub async fn start_capture(
    handle: AppHandle,
    device_name: Option<String>,
) -> Result<(), String> {
    let engine = get_engine(&handle);
    spawn_capture_loop(engine, device_name).map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn stop_capture(handle: AppHandle) -> Result<(), String> {
    let engine = get_engine(&handle);
    *engine.is_running.lock() = false;
    Ok(())
}

#[tauri::command]
pub fn get_input_devices() -> Result<Vec<crate::audio::capture::DeviceInfo>, String> {
    AudioCapture::list_devices().map_err(|e| e.to_string())
}

// ── Station management commands ──

#[tauri::command]
pub async fn add_station(handle: AppHandle, station: StationInfo) -> Result<(), String> {
    let engine = get_engine(&handle);
    engine.add_station(station).map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn update_station(handle: AppHandle, id: String, station: StationInfo) -> Result<(), String> {
    let engine = get_engine(&handle);
    engine.update_station(&id, station).map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn remove_station(handle: AppHandle, id: String) -> Result<(), String> {
    let engine = get_engine(&handle);
    engine.remove_station(&id).map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn connect_station(handle: AppHandle, id: String) -> Result<(), String> {
    let engine = get_engine(&handle);
    let result = engine.encoder_manager.lock().connect_station(&id);
    result.map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn disconnect_station(handle: AppHandle, id: String) -> Result<(), String> {
    let engine = get_engine(&handle);
    engine.encoder_manager.lock().disconnect_station(&id);
    Ok(())
}

#[tauri::command]
pub async fn connect_all_stations(handle: AppHandle) -> Result<(), String> {
    let engine = get_engine(&handle);
    let result = engine.encoder_manager.lock().connect_all();
    result.map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn disconnect_all_stations(handle: AppHandle) -> Result<(), String> {
    let engine = get_engine(&handle);
    engine.encoder_manager.lock().disconnect_all();
    Ok(())
}

// ── Metadata commands ──

#[tauri::command]
pub async fn set_metadata_source(
    handle: AppHandle,
    source: MetadataSource,
) -> Result<(), String> {
    let engine = get_engine(&handle);
    *engine.metadata_source.lock() = source;
    Ok(())
}

// ── Recording commands ──

#[tauri::command]
pub async fn start_recording(handle: AppHandle, path: String) -> Result<(), String> {
    let engine = get_engine(&handle);
    *engine.recording_path.lock() = Some(path);
    *engine.recording_active.lock() = true;
    Ok(())
}

#[tauri::command]
pub async fn stop_recording(handle: AppHandle) -> Result<(), String> {
    let engine = get_engine(&handle);
    *engine.recording_active.lock() = false;
    *engine.recording_path.lock() = None;
    Ok(())
}

// ── Stats command ──

#[tauri::command]
pub async fn get_encoder_stats(handle: AppHandle) -> Result<EngineState, String> {
    let engine = get_engine(&handle);
    Ok(engine.build_state())
}

// ── DSP commands ──

#[tauri::command]
pub async fn set_eq_band(
    handle: AppHandle,
    band: usize,
    gain_db: f32,
) -> Result<(), String> {
    let engine = get_engine(&handle);
    let mut mixer = engine.mixer.lock();
    mixer.equalizer_l.set_gain(band, gain_db);
    mixer.equalizer_r.set_gain(band, gain_db);
    Ok(())
}

#[tauri::command]
pub async fn set_compressor(
    handle: AppHandle,
    threshold_db: f32,
    ratio: f32,
    attack_ms: f32,
    release_ms: f32,
    makeup_gain_db: f32,
) -> Result<(), String> {
    let engine = get_engine(&handle);
    let mut mixer = engine.mixer.lock();
    mixer.compressor_l.set_params(threshold_db, ratio, attack_ms, release_ms, makeup_gain_db);
    mixer.compressor_r.set_params(threshold_db, ratio, attack_ms, release_ms, makeup_gain_db);
    Ok(())
}

#[tauri::command]
pub async fn set_limiter(
    handle: AppHandle,
    ceiling_db: f32,
    release_ms: f32,
) -> Result<(), String> {
    let engine = get_engine(&handle);
    let mut mixer = engine.mixer.lock();
    mixer.limiter_l.set_params(ceiling_db, release_ms);
    mixer.limiter_r.set_params(ceiling_db, release_ms);
    Ok(())
}
