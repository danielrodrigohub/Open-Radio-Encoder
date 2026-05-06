#pragma once
#include "encoder_interface.h"
namespace ore {
class OpusEncoder_ : public IEncoder {
public:
    int init(const EncoderConfig& config) override;
    int encode(const float* pcm_in, int num_samples, uint8_t* enc_out, int max_out_bytes) override;
    void close() override;
    CodecType type() const override { return CodecType::Opus; }
    int outputSamplerate() const override { return 48000; } // Opus always outputs 48kHz
};
} // namespace ore
