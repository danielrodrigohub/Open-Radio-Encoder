#pragma once
#include "encoder_interface.h"
namespace ore {
class VorbisEncoder : public IEncoder {
public:
    int init(const EncoderConfig& config) override;
    int encode(const float* pcm_in, int num_samples, uint8_t* enc_out, int max_out_bytes) override;
    void close() override;
    CodecType type() const override { return CodecType::Vorbis; }
};
} // namespace ore
