use cpal::traits::{DeviceTrait, HostTrait, StreamTrait};
use cpal::{BufferSize, SampleFormat, Stream, StreamConfig};
use crossbeam_channel::{bounded, Receiver};

pub struct AudioCapture {
    pub stream: Stream,
    pub sample_rx: Receiver<Vec<f32>>,
    pub device_name: String,
    pub sample_rate: u32,
    pub channels: u16,
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
        let supported = device.default_input_config()?;
        let channels = supported.channels();
        let sample_rate = supported.sample_rate().0;
        let sample_format = supported.sample_format();

        log::info!(
            "Using input device: {} ({} ch, {} Hz, {:?})",
            actual_name, channels, sample_rate, sample_format
        );

        // Use device's native channels and sample rate, always request F32
        let config: StreamConfig = StreamConfig {
            channels,
            sample_rate: cpal::SampleRate(sample_rate),
            buffer_size: BufferSize::Default,
        };

        let (sample_tx, sample_rx) = bounded::<Vec<f32>>(8);

        let stream = device.build_input_stream(
            &config,
            move |data: &[f32], _: &cpal::InputCallbackInfo| {
                let buffer = data.to_vec();
                let _ = sample_tx.try_send(buffer);
            },
            |err| {
                log::error!("Audio capture error: {}", err);
            },
            None,
        )?;

        stream.play()?;

        log::info!("Audio capture started: {} Hz, {} ch", sample_rate, channels);

        Ok(Self {
            stream,
            sample_rx,
            device_name: actual_name,
            sample_rate,
            channels,
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
