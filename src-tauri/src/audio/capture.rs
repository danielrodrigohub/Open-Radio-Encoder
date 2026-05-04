use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use cpal::{BufferSize, SampleFormat, Stream, StreamConfig};
use crossbeam_channel::{bounded, Receiver, Sender};
use dasp::Signal;
use std::sync::Arc;

const SAMPLE_RATE: u32 = 44100;
const CHANNELS: u16 = 2;
const BUFFER_FRAMES: u32 = 1024;

pub struct AudioCapture {
    pub stream: Stream,
    pub sample_rx: Receiver<Vec<f32>>,
    pub device_name: String,
    pub sample_rate: u32,
}

impl AudioCapture {
    pub fn new(device_name: Option<String>) -> anyhow::Result<Self> {
        let host = cpal::default_host();

        let device = match device_name {
            Some(ref name) => host
                .input_devices()?
                .find(|d| d.name().map(|n| n == *name).unwrap_or(false))
                .ok_or_else(|| anyhow::anyhow!("Input device '{}' not found", name))?,
            None => host
                .default_input_device()
                .ok_or_else(|| anyhow::anyhow!("No default input device found"))?,
        };

        let actual_name = device.name()?;
        log::info!("Using input device: {}", actual_name);

        let supported_config = device.default_input_config()?;
        let sample_rate = supported_config.sample_rate().0;

        // Always use F32 for DSP processing
        let config: StreamConfig = StreamConfig {
            channels: CHANNELS,
            sample_rate: cpal::SampleRate(SAMPLE_RATE),
            buffer_size: BufferSize::Fixed(BUFFER_FRAMES),
        };

        let (sample_tx, sample_rx) = bounded::<Vec<f32>>(8); // 8-buffer ring

        let stream = device.build_input_stream(
            &config,
            move |data: &[f32], _: &cpal::InputCallbackInfo| {
                // Copy audio data to avoid holding the callback
                let buffer = data.to_vec();
                // Non-blocking send; drop if consumer is behind
                let _ = sample_tx.try_send(buffer);
            },
            |err| {
                log::error!("Audio capture error: {}", err);
            },
            None,
        )?;

        stream.play()?;

        log::info!(
            "Audio capture started: {} Hz, {} channels, {} frames buffer",
            SAMPLE_RATE,
            CHANNELS,
            BUFFER_FRAMES
        );

        Ok(Self {
            stream,
            sample_rx,
            device_name: actual_name,
            sample_rate,
        })
    }

    pub fn list_devices() -> anyhow::Result<Vec<DeviceInfo>> {
        let host = cpal::default_host();
        let devices = host
            .input_devices()?
            .filter_map(|d| {
                let name = d.name().ok()?;
                let config = d.default_input_config().ok()?;
                Some(DeviceInfo {
                    name,
                    channels: config.channels(),
                    sample_rate: config.sample_rate().0,
                    is_default: d.name().ok().map_or(false, |n| {
                        host.default_input_device()
                            .and_then(|def| def.name().ok())
                            .map_or(false, |def_n| def_n == n)
                    }),
                })
            })
            .collect();
        Ok(devices)
    }
}

#[derive(Debug, Clone, serde::Serialize, serde::Deserialize)]
pub struct DeviceInfo {
    pub name: String,
    pub channels: u16,
    pub sample_rate: u32,
    pub is_default: bool,
}
