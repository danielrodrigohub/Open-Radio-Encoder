pub mod audio;
pub mod server;

use audio::engine::AudioEngine;

#[cfg_attr(mobile, tauri::mobile_entry_point)]
pub fn run() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .plugin(tauri_plugin_dialog::init())
        .invoke_handler(tauri::generate_handler![
            audio::commands::start_capture,
            audio::commands::stop_capture,
            audio::commands::get_input_devices,
            audio::commands::add_station,
            audio::commands::update_station,
            audio::commands::remove_station,
            audio::commands::connect_station,
            audio::commands::disconnect_station,
            audio::commands::connect_all_stations,
            audio::commands::disconnect_all_stations,
            audio::commands::set_metadata_source,
            audio::commands::start_recording,
            audio::commands::stop_recording,
            audio::commands::get_encoder_stats,
            audio::commands::set_eq_band,
            audio::commands::set_compressor,
            audio::commands::set_limiter,
            server::commands::start_remote_server,
            server::commands::stop_remote_server,
        ])
        .setup(|app| {
            let handle = app.handle().clone();
            AudioEngine::new(handle).expect("Failed to initialize audio engine");
            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("error while running Open Radio Encoder");
}
