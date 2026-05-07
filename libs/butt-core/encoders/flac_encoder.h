// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — FLAC Encoder
// Wraps libFLAC — adapted from BUTT 1.46.0 flac_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include "encoder_interface.h"

#ifdef HAVE_FLAC
#include <FLAC/stream_encoder.h>
#endif

#include <vector>

namespace ore {
class FlacEncoder : public IEncoder {
public:
    int init(const EncoderConfig& config) override;
    int encode(const float* pcm_in, int num_samples,
               uint8_t* enc_out, int max_out_bytes) override;
    int flush(uint8_t* enc_out, int max_out_bytes) override;
    void close() override;
    CodecType type() const override { return CodecType::FLAC; }

#ifdef HAVE_FLAC
    // Public: accessed by the static write callback
    FLAC__StreamEncoder* encoder_ = nullptr;
    std::vector<uint8_t>* outputPtr_ = nullptr;
#endif

private:
#ifdef HAVE_FLAC
    std::vector<FLAC__int32> pcmBuffer_;
#endif
};
} // namespace ore
