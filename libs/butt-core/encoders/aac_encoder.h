// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — AAC Encoder (stub)
// Wraps libfdk-aac - adapted from BUTT 1.46.0 aac_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include "encoder_interface.h"

namespace ore {
class AacEncoder : public IEncoder {
public:
    int init(const EncoderConfig& config) override;
    int encode(const float* pcm_in, int num_samples,
               uint8_t* enc_out, int max_out_bytes) override;
    void close() override;
    CodecType type() const override { return CodecType::AAC; }
};
} // namespace ore
