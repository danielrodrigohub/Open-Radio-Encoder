// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — TwoLAME MP2 Encoder Implementation
// NEW: MPEG Audio Layer 2 encoding via libtwolame
//
// TwoLAME API reference:
//   twolame_init()          — Create encoder options
//   twolame_set_*()         — Configure parameters
//   twolame_init_params()   — Finalize configuration
//   twolame_encode_buffer_float32_interleaved()
//   twolame_encode_flush()
//   twolame_close()
// ═══════════════════════════════════════════════════════════════════════
#include "twolame_encoder.h"
#include <cstdio>
#include <cstring>

namespace ore {

int TwoLameEncoder::init(const EncoderConfig& config) {
#ifdef HAVE_TWOLAME
    config_ = config;

    opts_ = twolame_init();
    if (!opts_) {
        fprintf(stderr, "[TwoLameEncoder] twolame_init() failed\n");
        return -1;
    }

    // Configure encoder parameters
    twolame_set_num_channels(opts_, config_.channels);
    twolame_set_in_samplerate(opts_, config_.samplerate);
    twolame_set_out_samplerate(opts_, config_.samplerate);
    twolame_set_bitrate(opts_, config_.bitrate);

    // MP2 mode: Joint Stereo is the standard for broadcast
    if (config_.channels == 2) {
        twolame_set_mode(opts_, TWOLAME_JOINT_STEREO);
    } else {
        twolame_set_mode(opts_, TWOLAME_MONO);
    }

    // MP2 standard samplerates: 32000, 44100, 48000
    // Energy level extensions (for broadcast compliance)
    twolame_set_energy_levels(opts_, 1);

    // Error protection (CRC) for broadcast reliability
    twolame_set_error_protection(opts_, TRUE);

    // Verbosity
    twolame_set_verbosity(opts_, 0);

    int rc = twolame_init_params(opts_);
    if (rc != 0) {
        fprintf(stderr, "[TwoLameEncoder] twolame_init_params() failed: %d\n", rc);
        twolame_close(&opts_);
        opts_ = nullptr;
        return rc;
    }

    fprintf(stdout, "[TwoLameEncoder] Initialized: %d Hz, %d ch, %d kbps MP2\n",
            config_.samplerate, config_.channels, config_.bitrate);

    return 0;
#else
    fprintf(stderr, "[TwoLameEncoder] Built without twolame support\n");
    return -1;
#endif
}

int TwoLameEncoder::encode(const float* pcm_in, int num_samples,
                            uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_TWOLAME
    if (!opts_ || num_samples == 0) return 0;

    // twolame_encode_buffer_float32_interleaved expects interleaved float32
    // samples in the range [-1.0, 1.0], which is exactly what PortAudio gives us.
    int rc = twolame_encode_buffer_float32_interleaved(
        opts_,
        pcm_in,
        num_samples,        // number of samples PER CHANNEL
        enc_out,
        max_out_bytes
    );

    if (rc < 0) {
        fprintf(stderr, "[TwoLameEncoder] Encode error: %d\n", rc);
    }
    return rc;
#else
    return 0;
#endif
}

int TwoLameEncoder::flush(uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_TWOLAME
    if (!opts_) return 0;
    return twolame_encode_flush(opts_, enc_out, max_out_bytes);
#else
    return 0;
#endif
}

void TwoLameEncoder::close() {
#ifdef HAVE_TWOLAME
    if (opts_) {
        twolame_close(&opts_);
        opts_ = nullptr;
    }
#endif
}

} // namespace ore
