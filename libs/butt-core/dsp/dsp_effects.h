// DSP Effects — refactored from BUTT dsp.hpp
#pragma once
#include <cstdint>

namespace ore {

class Biquad; // forward declare

class DSPEffects {
public:
    DSPEffects(uint32_t frames, uint8_t channels, uint32_t sampleRate);
    ~DSPEffects();

    bool hasToProcessSamples() const;
    void processSamples(float* audio_buf, uint32_t numSamples);
    void setEQband(int band, double gain_val);
    void setEQActive(bool active) { eq_active_ = active; }
    void setCompressorActive(bool active) { drc_active_ = active; }
    void resetCompressor();

    static constexpr int kBandCount = 10;

private:
    void compress(float* audio_buf, uint32_t numSamples);

    uint32_t samplerate_;
    uint8_t channels_;
    uint32_t dsp_size_;
    bool eq_active_ = false;
    bool drc_active_ = false;

    Biquad* eq_bands_left_[kBandCount] = {};
    Biquad* eq_bands_right_[kBandCount] = {};

    float attack_const_ = 0, release_const_ = 0, lowpass_const_ = 0;
    float prev_power_ = 1.0f;
    float prev_gain_dB_ = 0.0f;

    // Compressor parameters
    float threshold_ = -20.0f;
    float ratio_ = 4.0f;
    float attack_ = 0.01f;
    float release_ = 0.1f;
    float makeup_gain_ = 0.0f;
    float gain_ = 1.0f;
};

} // namespace ore
