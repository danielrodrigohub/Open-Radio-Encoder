use std::collections::HashMap;
use std::process::{Child, Command, Stdio};
use std::sync::Arc;
use parking_lot::Mutex;

use super::engine::{AudioCodec, StationInfo, StreamProtocol};

/// Encoding strategy for Open Radio Encoder:
///
/// | Codec      | Strategy                                          |
/// |------------|---------------------------------------------------|
/// | MP3        | mp3lame-encoder crate (Rust native)               |
/// | AAC/AAC+   | ffmpeg child process (wrapper over libfdk_aac)   |
/// | Opus       | opus crate (Rust native)                          |
/// | Ogg Vorbis | vorbis-encoder crate (Rust native)                |
/// | FLAC       | flac-encoder crate (Rust native)                  |
/// | WAV        | hound crate (Rust native)                         |
/// | MP2        | ffmpeg child process (wrapper over twolame)       |
///
/// For Icecast/Shoutcast streaming, ffmpeg can output directly to the
/// server URL using the icecast:// or shoutcast:// protocol prefix.
/// Alternatively, raw encoded data is piped through a custom HTTP source
/// client that implements the Icecast SOURCE protocol.

pub struct EncoderManager {
    encoders: HashMap<String, Arc<Mutex<StreamEncoder>>>,
}

pub struct StreamEncoder {
    pub station: StationInfo,
    ffmpeg_process: Option<Child>,
    pub active: bool,
}

impl EncoderManager {
    pub fn new() -> Self {
        Self {
            encoders: HashMap::new(),
        }
    }

    pub fn add_station(&mut self, station: StationInfo) {
        let encoder = StreamEncoder {
            station: station.clone(),
            ffmpeg_process: None,
            active: false,
        };
        self.encoders
            .insert(station.id.clone(), Arc::new(Mutex::new(encoder)));
    }

    pub fn remove_station(&mut self, id: &str) {
        if let Some(encoder) = self.encoders.get(id) {
            let mut enc = encoder.lock();
            if let Some(ref mut child) = enc.ffmpeg_process {
                let _ = child.kill();
                let _ = child.wait();
            }
        }
        self.encoders.remove(id);
    }

    pub fn connect_station(&self, id: &str) -> anyhow::Result<()> {
        let encoder = self
            .encoders
            .get(id)
            .ok_or_else(|| anyhow::anyhow!("Station {} not found", id))?;
        let mut enc = encoder.lock();
        enc.start_ffmpeg_stream()?;
        enc.active = true;
        Ok(())
    }

    pub fn disconnect_station(&self, id: &str) {
        if let Some(encoder) = self.encoders.get(id) {
            let mut enc = encoder.lock();
            if let Some(ref mut child) = enc.ffmpeg_process {
                let _ = child.kill();
                let _ = child.wait();
            }
            enc.ffmpeg_process = None;
            enc.active = false;
        }
    }

    pub fn connect_all(&self) -> anyhow::Result<()> {
        let ids: Vec<String> = self.encoders.keys().cloned().collect();
        for id in ids {
            self.connect_station(&id)?;
        }
        Ok(())
    }

    pub fn disconnect_all(&self) {
        let ids: Vec<String> = self.encoders.keys().cloned().collect();
        for id in ids {
            self.disconnect_station(&id);
        }
    }
}

impl StreamEncoder {
    fn build_ffmpeg_args(&self) -> Vec<String> {
        let mut args = vec![
            "-f".to_string(), "f32le".to_string(),
            "-ar".to_string(), "44100".to_string(),
            "-ac".to_string(), "2".to_string(),
            "-i".to_string(), "pipe:0".to_string(),
        ];

        // Codec-specific encoding parameters
        match self.station.codec {
            AudioCodec::MP3 => {
                args.extend_from_slice(&[
                    "-codec:a".to_string(), "libmp3lame".to_string(),
                    "-b:a".to_string(), format!("{}k", self.station.bitrate),
                ]);
            }
            AudioCodec::AAC => {
                args.extend_from_slice(&[
                    "-codec:a".to_string(), "libfdk_aac".to_string(),
                    "-profile:a".to_string(), "aac_low".to_string(),
                    "-b:a".to_string(), format!("{}k", self.station.bitrate),
                ]);
            }
            AudioCodec::AACPlus => {
                args.extend_from_slice(&[
                    "-codec:a".to_string(), "libfdk_aac".to_string(),
                    "-profile:a".to_string(), "aac_he_v2".to_string(),
                    "-b:a".to_string(), format!("{}k", self.station.bitrate),
                ]);
            }
            AudioCodec::Opus => {
                args.extend_from_slice(&[
                    "-codec:a".to_string(), "libopus".to_string(),
                    "-b:a".to_string(), format!("{}k", self.station.bitrate),
                    "-application".to_string(), "audio".to_string(),
                    "-vbr".to_string(), "on".to_string(),
                ]);
            }
            AudioCodec::OggVorbis => {
                args.extend_from_slice(&[
                    "-codec:a".to_string(), "libvorbis".to_string(),
                    "-qscale:a".to_string(), "5".to_string(),
                ]);
            }
            AudioCodec::MP2 => {
                args.extend_from_slice(&[
                    "-codec:a".to_string(), "mp2".to_string(),
                    "-b:a".to_string(), format!("{}k", self.station.bitrate),
                ]);
            }
            AudioCodec::FLAC | AudioCodec::WAV => {
                // FLAC/WAV are lossless, typically not used for streaming
                args.extend_from_slice(&[
                    "-codec:a".to_string(),
                    if matches!(self.station.codec, AudioCodec::FLAC) {
                        "flac".to_string()
                    } else {
                        "pcm_s16le".to_string()
                    },
                ]);
            }
        }

        // Output format based on protocol
        match self.station.protocol {
            StreamProtocol::Icecast2 => {
                let url = format!(
                    "icecast://source:{}@{}:{}/{}",
                    self.station.password,
                    self.station.host,
                    self.station.port,
                    self.station.mount.trim_start_matches('/')
                );
                args.extend_from_slice(&[
                    "-content_type".to_string(),
                    format!("audio/{}", self.content_type()),
                    "-ice_name".to_string(), self.station.name.clone(),
                    "-ice_description".to_string(), "Open Radio Encoder Stream".to_string(),
                    "-f".to_string(), "mp3".to_string(),
                    url,
                ]);
            }
            StreamProtocol::Shoutcast => {
                let url = format!(
                    "icecast://source:{}@{}:{}/{}",
                    self.station.password,
                    self.station.host,
                    self.station.port,
                    self.station.mount.trim_start_matches('/')
                );
                args.extend_from_slice(&[
                    "-content_type".to_string(), format!("audio/{}", self.content_type()),
                    "-f".to_string(), "mp3".to_string(),
                    url,
                ]);
            }
        }

        args
    }

    fn content_type(&self) -> &str {
        match self.station.codec {
            AudioCodec::MP3 => "mpeg",
            AudioCodec::AAC | AudioCodec::AACPlus => "aac",
            AudioCodec::Opus => "ogg",
            AudioCodec::OggVorbis => "ogg",
            AudioCodec::FLAC => "flac",
            AudioCodec::WAV => "wav",
            AudioCodec::MP2 => "mpeg",
        }
    }

    fn start_ffmpeg_stream(&mut self) -> anyhow::Result<()> {
        if self.ffmpeg_process.is_some() {
            return Ok(()); // Already running
        }

        let args = self.build_ffmpeg_args();

        log::info!(
            "Starting ffmpeg encoder for station '{}': ffmpeg {}",
            self.station.name,
            args.join(" ")
        );

        let child = Command::new("ffmpeg")
            .args(&args)
            .stdin(Stdio::piped())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .spawn()?;

        self.ffmpeg_process = Some(child);
        Ok(())
    }
}
