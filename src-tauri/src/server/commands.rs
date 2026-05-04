use crate::server::RemoteServer;

#[tauri::command]
pub async fn start_remote_server(
    port: u16,
    _smtp_config: Option<crate::server::SmtpConfig>,
    _webhook_urls: Vec<String>,
) -> Result<(), String> {
    let server = RemoteServer::new(port);
    server.start(_smtp_config, _webhook_urls).await
        .map_err(|e| e.to_string())
}

#[tauri::command]
pub async fn stop_remote_server(
) -> Result<(), String> {
    // In production, store server handle and call stop()
    Ok(())
}
