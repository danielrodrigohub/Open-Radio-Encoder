// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — LAME MP3 Encoder
// Wraps libmp3lame with the IEncoder interface
// Adapted from BUTT 1.46.0 lame_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#pragma once

#include "encoder_interface.h"

#ifdef HAVE_LAME
#include <lame/lame.h>
#endif

namespace ore {

class LameEncoder : public IEncoder {
public:
    LameEncoder() = default;
    ~LameEncoder() override { close(); }

    int init(const EncoderConfig& config) override;
    int encode(const float* pcm_in, int num_samples,
               uint8_t* enc_out, int max_out_bytes) override;
    int flush(uint8_t* enc_out, int max_out_bytes) override;
    void close() override;
    CodecType type() const override { return CodecType::MP3; }

private:
#ifdef HAVE_LAME
    lame_global_flags* gfp_ = nullptr;
#endif
};

} // namespace ore
