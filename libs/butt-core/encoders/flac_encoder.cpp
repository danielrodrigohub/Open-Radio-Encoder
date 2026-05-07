// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — FLAC Encoder Implementation
// Wraps libFLAC — adapted from BUTT 1.46.0 flac_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#include "flac_encoder.h"
#include <cstdio>
#include <cstring>

namespace ore {

#ifdef HAVE_FLAC
static FLAC__StreamEncoderWriteStatus flacWriteCallback(
    const FLAC__StreamEncoder* /*encoder*/,
    const FLAC__byte buffer[],
    size_t bytes,
    unsigned int /*samples*/,
    unsigned int /*current_frame*/,
    void* client_data) {
    auto* self = static_cast<FlacEncoder*>(client_data);
    if (self && self->outputPtr_) {
        self->outputPtr_->insert(self->outputPtr_->end(), buffer, buffer + bytes);
    }
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
}
#endif

int FlacEncoder::init(const EncoderConfig& config) {
#ifdef HAVE_FLAC
    config_ = config;

    encoder_ = FLAC__stream_encoder_new();
    if (!encoder_) {
        fprintf(stderr, "[FlacEncoder] Failed to create encoder\n");
        return -1;
    }

    FLAC__stream_encoder_set_channels(encoder_, config_.channels);
    FLAC__stream_encoder_set_sample_rate(encoder_, config_.samplerate);
    FLAC__stream_encoder_set_bits_per_sample(encoder_, 16);
    FLAC__stream_encoder_set_compression_level(encoder_, config_.quality);

    FLAC__StreamEncoderInitStatus status = FLAC__stream_encoder_init_stream(
        encoder_, flacWriteCallback, nullptr, nullptr, nullptr, this);

    if (status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
        fprintf(stderr, "[FlacEncoder] Init failed: %s\n",
                FLAC__StreamEncoderInitStatusString[status]);
        FLAC__stream_encoder_delete(encoder_);
        encoder_ = nullptr;
        return -1;
    }

    return 0;
#else
    (void)config;
    fprintf(stderr, "[FlacEncoder] Built without FLAC support\n");
    return -1;
#endif
}

int FlacEncoder::encode(const float* pcm_in, int num_samples,
                         uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_FLAC
    if (!encoder_ || num_samples == 0) return 0;

    int totalSamples = num_samples * config_.channels;

    // Convert float to int32 (FLAC uses 16-bit stored in 32-bit containers)
    pcmBuffer_.resize(totalSamples);
    for (int i = 0; i < totalSamples; i++) {
        float sample = pcm_in[i];
        sample = (sample < -1.0f) ? -1.0f : (sample > 1.0f) ? 1.0f : sample;
        pcmBuffer_[i] = static_cast<FLAC__int32>(sample * 32767.0f);
    }

    std::vector<uint8_t> outputVec;
    outputPtr_ = &outputVec;

    FLAC__bool ok = FLAC__stream_encoder_process_interleaved(
        encoder_, pcmBuffer_.data(), num_samples);

    outputPtr_ = nullptr;

    if (!ok) {
        fprintf(stderr, "[FlacEncoder] Encode error\n");
        return 0;
    }

    int bytesToWrite = static_cast<int>(outputVec.size());
    if (bytesToWrite > max_out_bytes) {
        bytesToWrite = max_out_bytes;
    }
    if (bytesToWrite > 0) {
        memcpy(enc_out, outputVec.data(), bytesToWrite);
    }

    return bytesToWrite;
#else
    (void)pcm_in; (void)num_samples; (void)enc_out; (void)max_out_bytes;
    return 0;
#endif
}

int FlacEncoder::flush(uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_FLAC
    if (!encoder_) return 0;

    std::vector<uint8_t> outputVec;
    outputPtr_ = &outputVec;

    FLAC__stream_encoder_finish(encoder_);

    outputPtr_ = nullptr;

    int bytesToWrite = static_cast<int>(outputVec.size());
    if (bytesToWrite > max_out_bytes) {
        bytesToWrite = max_out_bytes;
    }
    if (bytesToWrite > 0) {
        memcpy(enc_out, outputVec.data(), bytesToWrite);
    }

    return bytesToWrite;
#else
    (void)enc_out; (void)max_out_bytes;
    return 0;
#endif
}

void FlacEncoder::close() {
#ifdef HAVE_FLAC
    if (encoder_) {
        FLAC__stream_encoder_finish(encoder_);
        FLAC__stream_encoder_delete(encoder_);
        encoder_ = nullptr;
    }
    pcmBuffer_.clear();
#endif
}

} // namespace ore
