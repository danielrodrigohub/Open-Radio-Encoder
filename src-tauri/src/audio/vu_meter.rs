pub struct VuMeter {
    left_peak: f32,
    right_peak: f32,
    left_rms: f32,
    right_rms: f32,
    decay_rate: f32, // Peak hold decay per frame
    peak_hold_counter: u32,
    peak_hold_duration: u32,
}

impl VuMeter {
    pub fn new() -> Self {
        Self {
            left_peak: -54.0,
            right_peak: -54.0,
            left_rms: -54.0,
            right_rms: -54.0,
            decay_rate: 0.95,
            peak_hold_counter: 0,
            peak_hold_duration: 30, // ~1 second at 30fps
        }
    }

    /// Process a stereo interleaved buffer and update VU meter readings.
    /// Returns (left_dbfs, right_dbfs)
    pub fn process(&mut self, buffer: &[f32], channels: usize) -> (f32, f32) {
        if buffer.is_empty() || channels == 0 {
            // Apply decay to hold
            self.apply_decay();
            return (self.left_peak, self.right_peak);
        }

        let num_samples = buffer.len() / channels;

        let mut left_sum_sq = 0.0f64;
        let mut right_sum_sq = 0.0f64;
        let mut left_abs_max = 0.0f32;
        let mut right_abs_max = 0.0f32;

        for i in 0..num_samples {
            let left = buffer[i * channels];
            let right = if channels > 1 {
                buffer[i * channels + 1]
            } else {
                left
            };

            left_sum_sq += (left as f64) * (left as f64);
            right_sum_sq += (right as f64) * (right as f64);
            left_abs_max = left_abs_max.max(left.abs());
            right_abs_max = right_abs_max.max(right.abs());
        }

        let scale = 1.0 / num_samples as f64;
        let rms_left = (left_sum_sq * scale).sqrt() as f32;
        let rms_right = (right_sum_sq * scale).sqrt() as f32;

        let dbfs_left = db_fast(rms_left.max(1e-10));
        let dbfs_right = db_fast(rms_right.max(1e-10));
        let peak_left = db_fast(left_abs_max.max(1e-10));
        let peak_right = db_fast(right_abs_max.max(1e-10));

        // Peak hold logic
        if peak_left > self.left_peak {
            self.left_peak = peak_left;
            self.peak_hold_counter = 0;
        }
        if peak_right > self.right_peak {
            self.right_peak = peak_right;
            self.peak_hold_counter = 0;
        }

        self.peak_hold_counter += 1;
        if self.peak_hold_counter >= self.peak_hold_duration {
            self.apply_decay();
        }

        self.left_rms = dbfs_left;
        self.right_rms = dbfs_right;

        (self.left_peak, self.right_peak)
    }

    fn apply_decay(&mut self) {
        self.left_peak = self.left_peak * self.decay_rate;
        self.right_peak = self.right_peak * self.decay_rate;
        // Floor at -54 dB
        self.left_peak = self.left_peak.max(-54.0);
        self.right_peak = self.right_peak.max(-54.0);
    }

    pub fn get_levels(&self) -> (f32, f32) {
        (self.left_peak, self.right_peak)
    }

    pub fn get_rms(&self) -> (f32, f32) {
        (self.left_rms, self.right_rms)
    }
}

/// Fast approximate dB conversion for VU meter display
#[inline]
fn db_fast(amplitude: f32) -> f32 {
    // 20 * log10(x), clamped to -54dB minimum
    let db = 20.0 * amplitude.log10();
    db.max(-54.0)
}
