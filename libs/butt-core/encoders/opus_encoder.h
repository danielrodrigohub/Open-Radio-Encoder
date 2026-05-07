// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Opus Encoder
// Wraps libopus — adapted from BUTT 1.46.0 opus_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include "encoder_interface.h"

#include <vector>

#ifdef HAVE_OPUS
#include <opus/opus.h>
#include <ogg/ogg.h>
#endif

namespace ore {
class OpusEncoder_ : public IEncoder {
public:
    int init(const EncoderConfig& config) override;
    int encode(const float* pcm_in, int num_samples,
               uint8_t* enc_out, int max_out_bytes) override;
    void updateMetadata(const std::string& song) override;
    void close() override;
    CodecType type() const override { return CodecType::Opus; }
    int outputSamplerate() const override { return 48000; }

private:
#ifdef HAVE_OPUS
    ::OpusEncoder* encoder_ = nullptr;
    std::vector<float> pcmAccum_;

    ogg_stream_state os_;
    bool headersSent_ = false;
    ogg_int64_t packetCount_ = 0;
    ogg_int64_t granulePos_ = 0;

    int writePage(uint8_t* enc_out, int max_out_bytes, bool flush = false);
#endif
};
} // namespace ore
