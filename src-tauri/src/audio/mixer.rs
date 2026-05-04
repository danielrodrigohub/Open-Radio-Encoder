use biquad::*;
use std::collections::HashMap;

/// 10-band equalizer for audio processing
pub struct Equalizer {
    bands: Vec<Biquad<f32>>,
    gains_db: [f32; 10],
    sample_rate: u32,
}

impl Equalizer {
    /// Standard 10-band EQ frequencies
    pub const BAND_FREQUENCIES: [f32; 10] = [
        31.0, 63.0, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0,
    ];

    pub fn new(sample_rate: u32) -> Self {
        let fs = sample_rate as f32;
        let bands: Vec<Biquad<f32>> = Self::BAND_FREQUENCIES
            .iter()
            .map(|&freq| {
                // Peaking EQ with Q=1.4 (standard for graphic EQ)
                let coeffs = Coefficients::<f32>::from_params(
                    Type::PeakingEQ,
                    fs.hz(),
                    freq.hz(),
                    1.4f32.hz().into(),
                    0.0.db(),
                )
                .unwrap();
                Biquad::<f32>::from_coeffs(coeffs)
            })
            .collect();

        Self {
            bands,
            gains_db: [0.0f32; 10],
            sample_rate,
        }
    }

    pub fn set_gain(&mut self, band: usize, gain_db: f32) {
        if band >= 10 {
            return;
        }
        self.gains_db[band] = gain_db.clamp(-15.0, 15.0);

        let fs = self.sample_rate as f32;
        let freq = Self::BAND_FREQUENCIES[band];
        let coeffs = Coefficients::<f32>::from_params(
            Type::PeakingEQ,
            fs.hz(),
            freq.hz(),
            1.4f32.hz().into(),
            gain_db.db(),
        )
        .unwrap();
        self.bands[band] = Biquad::<f32>::from_coeffs(coeffs);
    }

    pub fn get_gains(&self) -> [f32; 10] {
        self.gains_db
    }

    /// Process a mono sample through all 10 bands
    pub fn process_sample(&mut self, sample: f32) -> f32 {
        let mut out = sample;
        for band in self.bands.iter_mut() {
            out = band.run(out);
        }
        out
    }
}

/// Simple compressor with threshold, ratio, attack, release
pub struct Compressor {
    threshold_db: f32,
    ratio: f32,
    attack_coeff: f32,
    release_coeff: f32,
    envelope: f32,
    makeup_gain: f32,
}

impl Compressor {
    pub fn new(sample_rate: u32) -> Self {
        Self {
            threshold_db: -18.0,
            ratio: 4.0,
            attack_coeff: (-1.0 / (0.010 * sample_rate as f32)).exp(),
            release_coeff: (-1.0 / (0.100 * sample_rate as f32)).exp(),
            envelope: 0.0,
            makeup_gain: 0.0,
        }
    }

    pub fn set_params(&mut self, threshold_db: f32, ratio: f32, attack_ms: f32, release_ms: f32, makeup_gain_db: f32) {
        self.threshold_db = threshold_db;
        self.ratio = ratio;
        self.attack_coeff = (-1.0 / (attack_ms / 1000.0 * 44100.0)).exp();
        self.release_coeff = (-1.0 / (release_ms / 1000.0 * 44100.0)).exp();
        self.makeup_gain = 10.0f32.powf(makeup_gain_db / 20.0);
    }

    pub fn process_sample(&mut self, sample: f32) -> f32 {
        let abs_sample = sample.abs().max(1e-10);
        let level_db = 20.0 * abs_sample.log10();

        let over_db = level_db - self.threshold_db;
        let target_reduction = if over_db > 0.0 {
            over_db * (1.0 - 1.0 / self.ratio)
        } else {
            0.0
        };

        // Attack/release envelope
        let coeff = if target_reduction > self.envelope {
            self.attack_coeff
        } else {
            self.release_coeff
        };

        self.envelope = coeff * self.envelope + (1.0 - coeff) * target_reduction;

        let gain_reduction = 10.0f32.powf(-self.envelope / 20.0);
        sample * gain_reduction * self.makeup_gain
    }
}

/// Brickwall limiter
pub struct Limiter {
    ceiling_db: f32,
    release_coeff: f32,
    envelope: f32,
}

impl Limiter {
    pub fn new(sample_rate: u32) -> Self {
        Self {
            ceiling_db: -0.3,
            release_coeff: (-1.0 / (0.050 * sample_rate as f32)).exp(),
            envelope: 0.0,
        }
    }

    pub fn set_params(&mut self, ceiling_db: f32, release_ms: f32) {
        self.ceiling_db = ceiling_db;
        self.release_coeff = (-1.0 / (release_ms / 1000.0 * 44100.0)).exp();
    }

    pub fn process_sample(&mut self, sample: f32) -> f32 {
        let abs_sample = sample.abs();
        let threshold = 10.0f32.powf(self.ceiling_db / 20.0);

        let over = abs_sample - threshold;
        if over > 0.0 {
            self.envelope = self.envelope.max(over);
        } else {
            self.envelope *= self.release_coeff;
        }

        if self.envelope > 0.0 {
            let reduction = 1.0 - (self.envelope / (abs_sample.max(1e-10)));
            sample * reduction.max(0.0)
        } else {
            sample
        }
    }
}

/// Complete audio signal chain: EQ → Compressor → Limiter
pub struct AudioMixer {
    pub equalizer: Equalizer,
    pub compressor: Compressor,
    pub limiter: Limiter,
    pub vst_chain: Vec<()>, // Placeholder for VST3 plugin instances
}

impl AudioMixer {
    pub fn new() -> Self {
        Self {
            equalizer: Equalizer::new(44100),
            compressor: Compressor::new(44100),
            limiter: Limiter::new(44100),
            vst_chain: Vec::new(),
        }
    }

    /// Process a full stereo interleaved buffer through the DSP chain
    pub fn process_buffer(&mut self, buffer: &mut [f32], channels: usize) {
        for i in 0..buffer.len() {
            // Apply EQ
            let sample = self.equalizer.process_sample(buffer[i]);
            // Apply Compressor
            let sample = self.compressor.process_sample(sample);
            // Apply Limiter
            let sample = self.limiter.process_sample(sample);
            buffer[i] = sample.clamp(-1.0, 1.0);
        }
    }
}
