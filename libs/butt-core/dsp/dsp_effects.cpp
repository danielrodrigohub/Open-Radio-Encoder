#include "dsp_effects.h"
#include "biquad.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ore {

static const double kDefaultEQFrequencies[10] = {
    31.0, 62.0, 125.0, 250.0, 500.0,
    1000.0, 2000.0, 4000.0, 8000.0, 16000.0
};

DSPEffects::DSPEffects(uint32_t frames, uint8_t ch, uint32_t sr)
    : samplerate_(sr), channels_(ch), dsp_size_(frames)
{
    for (int i = 0; i < kBandCount; i++) {
        double freq = kDefaultEQFrequencies[i];
        eq_bands_left_[i]  = new Biquad(Biquad::bq_type_peak, freq, 1.0, 0.0, static_cast<double>(sr));
        eq_bands_right_[i] = new Biquad(Biquad::bq_type_peak, freq, 1.0, 0.0, static_cast<double>(sr));
    }
    resetCompressor();
}

DSPEffects::~DSPEffects() {
    for (int i = 0; i < kBandCount; i++) {
        delete eq_bands_left_[i];
        delete eq_bands_right_[i];
    }
}

bool DSPEffects::hasToProcessSamples() const {
    return eq_active_ || drc_active_ || gain_ != 1.0f;
}

void DSPEffects::processSamples(float* audio_buf, uint32_t numSamples) {
    if (!hasToProcessSamples()) return;

    // Apply EQ
    if (eq_active_) {
        for (uint32_t s = 0; s < numSamples; s++) {
            float left  = audio_buf[s * channels_];
            float right = (channels_ >= 2) ? audio_buf[s * channels_ + 1] : left;

            float processedL = 0.0f, processedR = 0.0f;
            for (int b = 0; b < kBandCount; b++) {
                processedL += eq_bands_left_[b]->process(left);
                if (channels_ >= 2) {
                    processedR += eq_bands_right_[b]->process(right);
                }
            }

            if (channels_ >= 2) {
                // Mix: blend original with processed (gain controls amount)
                audio_buf[s * channels_]     = processedL;
                audio_buf[s * channels_ + 1] = processedR;
            } else {
                audio_buf[s] = processedL;
            }
        }
    }

    // Apply gain
    if (gain_ != 1.0f) {
        for (uint32_t i = 0; i < numSamples * channels_; i++) {
            audio_buf[i] *= gain_;
        }
    }

    // Apply compressor
    if (drc_active_) {
        compress(audio_buf, numSamples);
    }
}

void DSPEffects::setEQband(int band, double gain_val) {
    if (band < 0 || band >= kBandCount) return;
    eq_bands_left_[band]->setPeakGain(gain_val);
    eq_bands_right_[band]->setPeakGain(gain_val);
}

void DSPEffects::resetCompressor() {
    prev_power_ = 1.0f;
    prev_gain_dB_ = 0.0f;

    double attackTime = 0.010;   // 10 ms
    double releaseTime = 0.100;  // 100 ms
    double lowpassTime = 0.005;  // 5 ms RMS smoothing

    attack_const_  = static_cast<float>(std::exp(-1.0 / (attackTime * samplerate_)));
    release_const_ = static_cast<float>(std::exp(-1.0 / (releaseTime * samplerate_)));
    lowpass_const_ = static_cast<float>(1.0 - std::exp(-1.0 / (lowpassTime * samplerate_)));
}

void DSPEffects::compress(float* audio_buf, uint32_t numSamples) {
    if (!drc_active_) return;

    float threshold_linear = std::pow(10.0f, threshold_ / 20.0f);
    float slope = 1.0f - (1.0f / ratio_);
    float makeup_linear = std::pow(10.0f, makeup_gain_ / 20.0f);

    for (uint32_t s = 0; s < numSamples; s++) {
        // Compute RMS power (average of left and right squared)
        float left  = audio_buf[s * channels_];
        float right = (channels_ >= 2) ? audio_buf[s * channels_ + 1] : left;
        float power = (left * left + right * right) / 2.0f;

        // Low-pass filter the power (smoothing)
        float smoothed = prev_power_ + lowpass_const_ * (power - prev_power_);
        prev_power_ = smoothed;

        // Convert to dB
        float dB = 10.0f * std::log10(std::max(smoothed, 1e-10f));

        // Calculate gain reduction
        float targetGain_dB = 0.0f;
        if (dB > threshold_) {
            targetGain_dB = -(dB - threshold_) * slope;
        }

        // Apply attack/release smoothing
        if (targetGain_dB < prev_gain_dB_) {
            // Attack: gain is decreasing (compressing)
            prev_gain_dB_ = attack_const_ * prev_gain_dB_ + (1.0f - attack_const_) * targetGain_dB;
        } else {
            // Release: gain is recovering
            prev_gain_dB_ = release_const_ * prev_gain_dB_ + (1.0f - release_const_) * targetGain_dB;
        }

        float gain_linear = std::pow(10.0f, prev_gain_dB_ / 20.0f) * makeup_linear;

        audio_buf[s * channels_] = left * gain_linear;
        if (channels_ >= 2) {
            audio_buf[s * channels_ + 1] = right * gain_linear;
        }
    }
}

} // namespace ore
