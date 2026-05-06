// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — TwoLAME MP2 Encoder
// NEW: MPEG Layer 2 encoding via libtwolame
// ═══════════════════════════════════════════════════════════════════════
#pragma once

#include "encoder_interface.h"

#ifdef HAVE_TWOLAME
#include <twolame.h>
#endif

namespace ore {

class TwoLameEncoder : public IEncoder {
public:
    TwoLameEncoder() = default;
    ~TwoLameEncoder() override { close(); }

    int init(const EncoderConfig& config) override;
    int encode(const float* pcm_in, int num_samples,
               uint8_t* enc_out, int max_out_bytes) override;
    int flush(uint8_t* enc_out, int max_out_bytes) override;
    void close() override;
    CodecType type() const override { return CodecType::MP2; }

private:
#ifdef HAVE_TWOLAME
    twolame_options* opts_ = nullptr;
#endif
};

} // namespace ore
