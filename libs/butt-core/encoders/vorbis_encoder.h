// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Vorbis Encoder
// Wraps libvorbis/libvorbisenc — adapted from BUTT 1.46.0 vorbis_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include "encoder_interface.h"
#ifdef HAVE_VORBIS
#include <vorbis/vorbisenc.h>
#include <ogg/ogg.h>
#endif

namespace ore {
class VorbisEncoder : public IEncoder {
public:
    int init(const EncoderConfig& config) override;
    int encode(const float* pcm_in, int num_samples,
               uint8_t* enc_out, int max_out_bytes) override;
    int flush(uint8_t* enc_out, int max_out_bytes) override;
    void close() override;
    CodecType type() const override { return CodecType::Vorbis; }

private:
#ifdef HAVE_VORBIS
    vorbis_info      vi_;
    vorbis_comment   vc_;
    vorbis_dsp_state vd_;
    vorbis_block     vb_;

    ogg_stream_state os_;
    bool headersSent_ = false;
    bool initialized_ = false;

    int writePage(uint8_t* enc_out, int max_out_bytes, bool flush = false);
#endif
};
} // namespace ore
