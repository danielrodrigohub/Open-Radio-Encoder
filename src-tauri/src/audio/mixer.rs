use std::f32::consts::PI;

/// Biquad state variable filter - direct form I
#[derive(Clone, Copy)]
struct BiquadCoeffs {
    b0: f32,
    b1: f32,
    b2: f32,
    a1: f32,
    a2: f32,
}

struct BiquadState {
    coeffs: BiquadCoeffs,
    x1: f32,
    x2: f32,
    y1: f32,
    y2: f32,
}

impl BiquadState {
    fn process(&mut self, x: f32) -> f32 {
        let y = self.coeffs.b0 * x + self.coeffs.b1 * self.x1 + self.coeffs.b2 * self.x2
            - self.coeffs.a1 * self.y1 - self.coeffs.a2 * self.y2;
        self.x2 = self.x1;
        self.x1 = x;
        self.y2 = self.y1;
        self.y1 = y;
        y
    }
}

/// Calculate RBJ peaking EQ coefficients
fn peaking_eq_coeffs(sample_rate: f32, freq: f32, q: f32, gain_db: f32) -> BiquadCoeffs {
    let a = 10.0f32.powf(gain_db / 40.0);
    let w0 = 2.0 * PI * freq / sample_rate;
    let alpha = w0.sin() / (2.0 * q);

    let b0 = 1.0 + alpha * a;
    let b1 = -2.0 * w0.cos();
    let b2 = 1.0 - alpha * a;
    let a0 = 1.0 + alpha / a;
    let a1 = -2.0 * w0.cos();
    let a2 = 1.0 - alpha / a;

    BiquadCoeffs {
        b0: b0 / a0,
        b1: b1 / a0,
        b2: b2 / a0,
        a1: a1 / a0,
        a2: a2 / a0,
    }
}

/// 10-band graphic equalizer
pub struct Equalizer {
    bands: Vec<BiquadState>,
    gains_db: [f32; 10],
    sample_rate: u32,
}

impl Equalizer {
    /// Standard 10-band EQ frequencies
    pub const BAND_FREQUENCIES: [f32; 10] = [
        31.0, 63.0, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0, 16000.0,
    ];

    pub fn new(sample_rate: u32) -> Self {
        let bands: Vec<BiquadState> = Self::BAND_FREQUENCIES
            .iter()
            .map(|&freq| {
                let coeffs = peaking_eq_coeffs(sample_rate as f32, freq, 1.4, 0.0);
                BiquadState {
                    coeffs,
                    x1: 0.0,
                    x2: 0.0,
                    y1: 0.0,
                    y2: 0.0,
                }
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
        let coeffs = peaking_eq_coeffs(
            self.sample_rate as f32,
            Self::BAND_FREQUENCIES[band],
            1.4,
            self.gains_db[band],
        );
        self.bands[band].coeffs = coeffs;
    }

    pub fn get_gains(&self) -> [f32; 10] {
        self.gains_db
    }

    /// Process a mono sample through all 10 bands
    pub fn process_sample(&mut self, sample: f32) -> f32 {
        let mut out = sample;
        for band in self.bands.iter_mut() {
            out = band.process(out);
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
            makeup_gain: 1.0,
        }
    }

    pub fn set_params(
        &mut self,
        threshold_db: f32,
        ratio: f32,
        attack_ms: f32,
        release_ms: f32,
        makeup_gain_db: f32,
    ) {
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
    pub equalizer_l: Equalizer,
    pub equalizer_r: Equalizer,
    pub compressor_l: Compressor,
    pub compressor_r: Compressor,
    pub limiter_l: Limiter,
    pub limiter_r: Limiter,
}

impl AudioMixer {
    pub fn new() -> Self {
        let sample_rate = 44100;
        Self {
            equalizer_l: Equalizer::new(sample_rate),
            equalizer_r: Equalizer::new(sample_rate),
            compressor_l: Compressor::new(sample_rate),
            compressor_r: Compressor::new(sample_rate),
            limiter_l: Limiter::new(sample_rate),
            limiter_r: Limiter::new(sample_rate),
        }
    }

    /// Process a full stereo interleaved buffer through the DSP chain
    pub fn process_buffer(&mut self, buffer: &mut [f32], channels: usize) {
        // Sanitize input: replace NaNs/Infs with 0.0
        for sample in buffer.iter_mut() {
            if !sample.is_finite() {
                *sample = 0.0;
            }
        }

        if channels == 1 {
            for sample in buffer.iter_mut() {
                let s = self.equalizer_l.process_sample(*sample);
                let s = self.compressor_l.process_sample(s);
                let s = self.limiter_l.process_sample(s);
                *sample = s.clamp(-1.0, 1.0);
            }
        } else if channels == 2 {
            for i in (0..buffer.len()).step_by(2) {
                if i + 1 < buffer.len() {
                    // Left channel
                    let mut l = buffer[i];
                    l = self.equalizer_l.process_sample(l);
                    l = self.compressor_l.process_sample(l);
                    l = self.limiter_l.process_sample(l);
                    buffer[i] = l.clamp(-1.0, 1.0);

                    // Right channel
                    let mut r = buffer[i + 1];
                    r = self.equalizer_r.process_sample(r);
                    r = self.compressor_r.process_sample(r);
                    r = self.limiter_r.process_sample(r);
                    buffer[i + 1] = r.clamp(-1.0, 1.0);
                }
            }
        }

        // Sanitize output just in case filters produced invalid values
        for sample in buffer.iter_mut() {
            if !sample.is_finite() {
                *sample = 0.0;
            }
        }
    }
}
