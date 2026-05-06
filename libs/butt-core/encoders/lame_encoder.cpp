// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — LAME MP3 Encoder Implementation
// Adapted from BUTT 1.46.0 lame_encode.cpp by Daniel Noethen
// ═══════════════════════════════════════════════════════════════════════
#include "lame_encoder.h"
#include <cstdio>

namespace ore {

int LameEncoder::init(const EncoderConfig& config) {
#ifdef HAVE_LAME
    config_ = config;
    gfp_ = lame_init();
    if (!gfp_) return -1;

    lame_set_num_channels(gfp_, config_.channels);
    lame_set_in_samplerate(gfp_, config_.samplerate);
    lame_set_out_samplerate(gfp_, config_.samplerate);
    lame_set_brate(gfp_, config_.bitrate);
    lame_set_quality(gfp_, config_.quality);

    switch (config_.bitrate_mode) {
        case 0: lame_set_VBR(gfp_, vbr_off);     break;  // CBR
        case 1: lame_set_VBR(gfp_, vbr_default);  break;  // VBR
        case 2: lame_set_VBR(gfp_, vbr_abr);      break;  // ABR
    }

    if (config_.bitrate_mode > 0) {
        lame_set_VBR_mean_bitrate_kbps(gfp_, config_.bitrate);
    }

    int rc = lame_init_params(gfp_);
    if (rc < 0) {
        fprintf(stderr, "[LameEncoder] lame_init_params failed: %d\n", rc);
        lame_close(gfp_);
        gfp_ = nullptr;
        return rc;
    }

    return 0;
#else
    fprintf(stderr, "[LameEncoder] Built without LAME support\n");
    return -1;
#endif
}

int LameEncoder::encode(const float* pcm_in, int num_samples,
                         uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_LAME
    if (!gfp_ || num_samples == 0) return 0;

    int rc;
    if (config_.channels == 2) {
        rc = lame_encode_buffer_interleaved_ieee_float(
            gfp_, pcm_in, num_samples,
            enc_out, max_out_bytes);
    } else {
        rc = lame_encode_buffer_ieee_float(
            gfp_, pcm_in, pcm_in, num_samples,
            enc_out, max_out_bytes);
    }
    return rc;
#else
    return 0;
#endif
}

int LameEncoder::flush(uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_LAME
    if (!gfp_) return 0;
    return lame_encode_flush(gfp_, enc_out, max_out_bytes);
#else
    return 0;
#endif
}

void LameEncoder::close() {
#ifdef HAVE_LAME
    if (gfp_) {
        lame_close(gfp_);
        gfp_ = nullptr;
    }
#endif
}

} // namespace ore
