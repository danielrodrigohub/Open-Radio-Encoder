pub mod commands;

use std::io::Read;
use std::sync::Arc;
use parking_lot::Mutex;

pub struct RemoteServer {
    running: Arc<Mutex<bool>>,
    port: u16,
}

impl RemoteServer {
    pub fn new(port: u16) -> Self {
        Self {
            running: Arc::new(Mutex::new(false)),
            port,
        }
    }

    pub async fn start(&self, smtp_config: Option<SmtpConfig>, webhook_urls: Vec<String>) -> anyhow::Result<()> {
        let running = self.running.clone();
        *running.lock() = true;
        let port = self.port;

        tokio::spawn(async move {
            log::info!("Remote control server starting on port {}", port);

            // Tiny HTTP server for remote control endpoints
            let addr = format!("0.0.0.0:{}", port);
            let server = match tiny_http::Server::http(&addr) {
                Ok(s) => s,
                Err(e) => {
                    log::error!("Failed to start remote server: {}", e);
                    return;
                }
            };

            while *running.lock() {
                if let Some(request) = server.recv_timeout(std::time::Duration::from_secs(1)) {
                    let url = request.url().to_string();
                    let response = match url.as_str() {
                        "/api/start" => {
                            log::info!("Remote: START command received");
                            "OK: Start command received".to_string()
                        }
                        "/api/stop" => {
                            log::info!("Remote: STOP command received");
                            "OK: Stop command received".to_string()
                        }
                        "/api/status" => {
                            "OK: Running".to_string()
                        }
                        "/api/metadata" => {
                            // Parse body for metadata update
                            let mut body = String::new();
                            let _ = request.as_reader().read_to_string(&mut body);
                            log::info!("Remote: Metadata update: {}", body);
                            "OK: Metadata updated".to_string()
                        }
                        _ => {
                            format!("Unknown endpoint: {}", url)
                        }
                    };

                    let resp = tiny_http::Response::from_string(response);
                    let _ = request.respond(resp);
                }
            }

            log::info!("Remote control server stopped");
        });

        Ok(())
    }

    pub fn stop(&self) {
        *self.running.lock() = false;
    }
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct SmtpConfig {
    pub server: String,
    pub port: u16,
    pub username: String,
    pub password: String,
    pub from_email: String,
    pub to_email: String,
}

/// Send notification via SMTP for stream status changes
pub fn send_smtp_notification(config: &SmtpConfig, subject: &str, body: &str) -> anyhow::Result<()> {
    use lettre::transport::smtp::authentication::Credentials;
    use lettre::{Message, SmtpTransport, Transport};

    let email = Message::builder()
        .from(config.from_email.parse()?)
        .to(config.to_email.parse()?)
        .subject(subject)
        .body(body.to_string())?;

    let creds = Credentials::new(config.username.clone(), config.password.clone());

    let mailer = SmtpTransport::relay(&config.server)?
        .port(config.port)
        .credentials(creds)
        .build();

    mailer.send(&email)?;
    Ok(())
}

/// Send webhook notification on stream state changes
pub async fn send_webhook(url: &str, payload: &serde_json::Value) -> anyhow::Result<()> {
    let client = reqwest::Client::new();
    client.post(url)
        .json(payload)
        .send()
        .await?;
    Ok(())
}
