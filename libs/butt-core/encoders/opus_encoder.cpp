// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Opus Encoder Implementation
// Wraps libopus — adapted from BUTT 1.46.0 opus_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#include "opus_encoder.h"
#include <cstdio>
#include <cstring>

namespace ore {

int OpusEncoder_::init(const EncoderConfig& config) {
#ifdef HAVE_OPUS
    config_ = config;
    // Opus only supports 48000 Hz internally
    int err;
    encoder_ = opus_encoder_create(48000, config_.channels, OPUS_APPLICATION_AUDIO, &err);
    if (err != OPUS_OK || !encoder_) {
        fprintf(stderr, "[OpusEncoder] Failed to create encoder: %d\n", err);
        encoder_ = nullptr;
        return -1;
    }

    opus_encoder_ctl(encoder_, OPUS_SET_BITRATE(config_.bitrate * 1000));
    opus_encoder_ctl(encoder_, OPUS_SET_COMPLEXITY(10));

    // Initialize Ogg stream
    static int serial = 0;
    ogg_stream_init(&os_, serial++);
    headersSent_ = false;
    packetCount_ = 0;
    granulePos_ = 0;

    return 0;
#else
    (void)config;
    fprintf(stderr, "[OpusEncoder] Built without Opus support\n");
    return -1;
#endif
}

 #ifdef HAVE_OPUS
int OpusEncoder_::writePage(uint8_t* enc_out, int max_out_bytes, bool flush) {
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

int OpusEncoder_::encode(const float* pcm_in, int num_samples,
                          uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_OPUS
    if (!encoder_ || num_samples == 0) return 0;

    int totalWritten = 0;

    // ── Phase 1: Headers ──
    if (!headersSent_) {
        // OpusHead
        uint8_t head[19];
        memcpy(head, "OpusHead", 8);
        head[8] = 1; // version
        head[9] = config_.channels;
        uint16_t preskip = 3840; // Default preskip
        memcpy(head + 10, &preskip, 2);
        uint32_t rate = 48000;
        memcpy(head + 12, &rate, 4);
        head[16] = 0; // gain
        head[17] = 0; // mapping family
        
        ogg_packet op;
        op.packet = head;
        op.bytes = 19;
        op.b_o_s = 1;
        op.e_o_s = 0;
        op.granulepos = 0;
        op.packetno = packetCount_++;
        ogg_stream_packetin(&os_, &op);
        totalWritten += writePage(enc_out, max_out_bytes, true);

        // OpusTags
        const char* vendor = "OpenRadioEncoder";
        int vendorLen = strlen(vendor);
        int tagsLen = 8 + 4 + vendorLen + 4;
        std::vector<uint8_t> tags(tagsLen);
        memcpy(tags.data(), "OpusTags", 8);
        memcpy(tags.data() + 8, &vendorLen, 4);
        memcpy(tags.data() + 12, vendor, vendorLen);
        uint32_t zero = 0;
        memcpy(tags.data() + 12 + vendorLen, &zero, 4);

        op.packet = tags.data();
        op.bytes = tagsLen;
        op.b_o_s = 0;
        op.granulepos = 0;
        op.packetno = packetCount_++;
        ogg_stream_packetin(&os_, &op);
        totalWritten += writePage(enc_out + totalWritten, max_out_bytes - totalWritten, true);

        headersSent_ = true;
    }

    // ── Phase 2: Audio ──
    int channels = config_.channels;
    int totalSamples = num_samples * channels;

    int oldSize = static_cast<int>(pcmAccum_.size());
    pcmAccum_.resize(oldSize + totalSamples);
    for (int i = 0; i < totalSamples; i++) {
        float s = pcm_in[i];
        s = (s < -1.0f) ? -1.0f : (s > 1.0f) ? 1.0f : s;
        pcmAccum_[oldSize + i] = s;
    }

    const int kFrameSamples = 960 * channels;
    while (static_cast<int>(pcmAccum_.size()) >= kFrameSamples) {
        uint8_t packet[2048]; // Max Opus packet size
        int encodedBytes = opus_encode_float(encoder_, pcmAccum_.data(), 960, packet, sizeof(packet));
        
        if (encodedBytes > 0) {
            granulePos_ += 960;
            ogg_packet op;
            op.packet = packet;
            op.bytes = encodedBytes;
            op.b_o_s = 0;
            op.e_o_s = 0;
            op.granulepos = granulePos_;
            op.packetno = packetCount_++;
            ogg_stream_packetin(&os_, &op);
            
            // Flush after every packet for real-time streaming
            totalWritten += writePage(enc_out + totalWritten, max_out_bytes - totalWritten, true);
        }
        
        pcmAccum_.erase(pcmAccum_.begin(), pcmAccum_.begin() + kFrameSamples);
    }

    return totalWritten;
#else
    (void)pcm_in; (void)num_samples; (void)enc_out; (void)max_out_bytes;
    return 0;
#endif
}

void OpusEncoder_::updateMetadata(const std::string& song) {
#ifdef HAVE_OPUS
    // For Opus in Ogg, we can't easily insert a new tag packet in the middle
    // of an existing logical stream without libshout or the server getting confused
    // unless we use chaining. For now, we rely on the out-of-band metadata fix.
    // However, we'll log it for future in-band implementation.
    fprintf(stdout, "[OpusEncoder] Metadata update: %s\n", song.c_str());
#endif
}

void OpusEncoder_::close() {
#ifdef HAVE_OPUS
    if (encoder_) {
        opus_encoder_destroy(encoder_);
        encoder_ = nullptr;
    }
    ogg_stream_clear(&os_);
    pcmAccum_.clear();
#endif
}

} // namespace ore
