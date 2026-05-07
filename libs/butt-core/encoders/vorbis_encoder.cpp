// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Vorbis Encoder Implementation
// Wraps libvorbis/libvorbisenc — adapted from BUTT 1.46.0 vorbis_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#include "vorbis_encoder.h"
#include <cstdio>
#include <cstring>

namespace ore {

int VorbisEncoder::init(const EncoderConfig& config) {
#ifdef HAVE_VORBIS
    config_ = config;

    vorbis_info_init(&vi_);

    int rc;
    if (config_.bitrate_mode == 1) {
        // VBR
        rc = vorbis_encode_init_vbr(&vi_, config_.channels, config_.samplerate,
                                     static_cast<float>(config_.quality) / 10.0f);
    } else {
        // CBR or ABR
        rc = vorbis_encode_init(&vi_, config_.channels, config_.samplerate,
                                 -1, config_.bitrate * 1000, -1);
    }

    if (rc != 0) {
        fprintf(stderr, "[VorbisEncoder] vorbis_encode_init failed: %d\n", rc);
        vorbis_info_clear(&vi_);
        return -1;
    }

    vorbis_comment_init(&vc_);
    vorbis_comment_add_tag(&vc_, "ENCODER", "OpenRadioEncoder");

    vorbis_analysis_init(&vd_, &vi_);
    vorbis_block_init(&vd_, &vb_);

    // Initialize Ogg stream
    static int serial = 0;
    ogg_stream_init(&os_, serial++);
    headersSent_ = false;
    initialized_ = true;
    return 0;
#else
    (void)config;
    fprintf(stderr, "[VorbisEncoder] Built without Vorbis support\n");
    return -1;
#endif
}

#ifdef HAVE_VORBIS
int VorbisEncoder::writePage(uint8_t* enc_out, int max_out_bytes, bool flush) {
    int totalBytes = 0;
    ogg_page og;
    auto getPage = flush ? ogg_stream_flush : ogg_stream_pageout;
    while (getPage(&os_, &og)) {
        if (totalBytes + og.header_len + og.body_len > max_out_bytes) break;
        memcpy(enc_out + totalBytes, og.header, og.header_len);
        totalBytes += og.header_len;
        memcpy(enc_out + totalBytes, og.body, og.body_len);
        totalBytes += og.body_len;
    }
    return totalBytes;
}
#endif

int VorbisEncoder::encode(const float* pcm_in, int num_samples,
                           uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_VORBIS
    if (!initialized_ || num_samples == 0) return 0;

    int totalWritten = 0;

    // ── Phase 1: Headers ──
    if (!headersSent_) {
        ogg_packet header;
        ogg_packet header_comm;
        ogg_packet header_code;

        vorbis_analysis_headerout(&vd_, &vc_, &header, &header_comm, &header_code);
        ogg_stream_packetin(&os_, &header);
        ogg_stream_packetin(&os_, &header_comm);
        ogg_stream_packetin(&os_, &header_code);

        totalWritten += writePage(enc_out, max_out_bytes, true);
        headersSent_ = true;
    }

    // ── Phase 2: Audio ──
    int channels = config_.channels;
    float** buffer = vorbis_analysis_buffer(&vd_, num_samples);
    for (int ch = 0; ch < channels; ch++) {
        for (int i = 0; i < num_samples; i++) {
            buffer[ch][i] = pcm_in[i * channels + ch];
        }
    }
    vorbis_analysis_wrote(&vd_, num_samples);

    ogg_packet op;
    while (vorbis_analysis_blockout(&vd_, &vb_) == 1) {
        vorbis_analysis(&vb_, nullptr);
        vorbis_bitrate_addblock(&vb_);

        while (vorbis_bitrate_flushpacket(&vd_, &op)) {
            ogg_stream_packetin(&os_, &op);
            // Flush after every packet for real-time streaming
            totalWritten += writePage(enc_out + totalWritten, max_out_bytes - totalWritten, true);
        }
    }

    return totalWritten;
#else
    (void)pcm_in; (void)num_samples; (void)enc_out; (void)max_out_bytes;
    return 0;
#endif
}

int VorbisEncoder::flush(uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_VORBIS
    if (!initialized_) return 0;

    vorbis_analysis_wrote(&vd_, 0);

    int totalWritten = 0;
    ogg_packet op;

    while (vorbis_analysis_blockout(&vd_, &vb_) == 1) {
        vorbis_analysis(&vb_, nullptr);
        vorbis_bitrate_addblock(&vb_);

        while (vorbis_bitrate_flushpacket(&vd_, &op)) {
            ogg_stream_packetin(&os_, &op);
            // Flush after every packet for real-time streaming
            totalWritten += writePage(enc_out + totalWritten, max_out_bytes - totalWritten, true);
        }
    }
    
    // Final flush
    totalWritten += writePage(enc_out + totalWritten, max_out_bytes - totalWritten, true);

    return totalWritten;
#else
    (void)enc_out; (void)max_out_bytes;
    return 0;
#endif
}

void VorbisEncoder::close() {
#ifdef HAVE_VORBIS
    if (initialized_) {
        vorbis_block_clear(&vb_);
        vorbis_dsp_clear(&vd_);
        vorbis_comment_clear(&vc_);
        vorbis_info_clear(&vi_);
        ogg_stream_clear(&os_);
        initialized_ = false;
        headersSent_ = false;
    }
#endif
}

} // namespace ore
