// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — AAC Encoder
// Wraps libfdk-aac - adapted from BUTT 1.46.0 aac_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include "encoder_interface.h"

#ifdef HAVE_LIBFDK_AAC
#include <fdk-aac/aacenc_lib.h>
#endif

namespace ore {
class AacEncoder : public IEncoder {
public:
    int init(const EncoderConfig& config) override;
    int encode(const float* pcm_in, int num_samples,
               uint8_t* enc_out, int max_out_bytes) override;
    int flush(uint8_t* enc_out, int max_out_bytes) override;
    void close() override;
    CodecType type() const override { return CodecType::AAC; }

private:
#ifdef HAVE_LIBFDK_AAC
    HANDLE_AACENCODER handle_ = nullptr;
    uint8_t* pcmBuffer_ = nullptr;
    int pcmBufferPos_ = 0;
    int pcmBufferCapacity_ = 0;
#endif
};
} // namespace ore
