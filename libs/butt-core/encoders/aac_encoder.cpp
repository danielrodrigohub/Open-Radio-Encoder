// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — AAC Encoder Implementation
// Wraps libfdk-aac — adapted from BUTT 1.46.0 aac_encode.cpp
// ═══════════════════════════════════════════════════════════════════════
#include "aac_encoder.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace ore {

int AacEncoder::init(const EncoderConfig& config) {
#ifdef HAVE_LIBFDK_AAC
    config_ = config;

    if (aacEncOpen(&handle_, 0, 0) != AACENC_OK) {
        fprintf(stderr, "[AacEncoder] Failed to open encoder\n");
        return -1;
    }

    // HE-AAC v1 (SBR)
    if (aacEncoder_SetParam(handle_, AACENC_AOT, 5) != AACENC_OK) {
        fprintf(stderr, "[AacEncoder] Failed to set AOT\n");
        return -1;
    }
    if (aacEncoder_SetParam(handle_, AACENC_SAMPLERATE, config_.samplerate) != AACENC_OK) {
        fprintf(stderr, "[AacEncoder] Failed to set sample rate\n");
        return -1;
    }
    if (aacEncoder_SetParam(handle_, AACENC_CHANNELMODE, config_.channels == 1 ? MODE_1 : MODE_2) != AACENC_OK) {
        fprintf(stderr, "[AacEncoder] Failed to set channel mode\n");
        return -1;
    }
    if (aacEncoder_SetParam(handle_, AACENC_BITRATE, config_.bitrate * 1000) != AACENC_OK) {
        fprintf(stderr, "[AacEncoder] Failed to set bitrate\n");
        return -1;
    }
    if (aacEncoder_SetParam(handle_, AACENC_TRANSMUX, TT_MP4_ADTS) != AACENC_OK) {
        fprintf(stderr, "[AacEncoder] Failed to set transport type\n");
        return -1;
    }

    if (aacEncEncode(handle_, nullptr, nullptr, nullptr, nullptr) != AACENC_OK) {
        fprintf(stderr, "[AacEncoder] Failed to initialize encoder\n");
        return -1;
    }

    pcmBufferCapacity_ = 16384 * sizeof(int16_t); // 16K samples capacity
    pcmBuffer_ = new uint8_t[pcmBufferCapacity_];
    pcmBufferPos_ = 0;

    return 0;
#else
    (void)config;
    fprintf(stderr, "[AacEncoder] Built without FDK-AAC support\n");
    return -1;
#endif
}

int AacEncoder::encode(const float* pcm_in, int num_samples,
                        uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_LIBFDK_AAC
    if (!handle_) return 0;

    // Convert float to int16 and accumulate
    int samplesToConvert = num_samples * config_.channels;
    int bytesToAppend = samplesToConvert * sizeof(int16_t);

    if (pcmBufferPos_ + bytesToAppend > pcmBufferCapacity_) {
        fprintf(stderr, "[AacEncoder] PCM Buffer Overflow! Resetting...\n");
        pcmBufferPos_ = 0;
    }

    for (int i = 0; i < samplesToConvert; i++) {
        float sample = pcm_in[i];
        sample = (std::max)(-1.0f, std::min(1.0f, sample));
        int16_t pcm = static_cast<int16_t>(sample * 32767.0f);
        memcpy(pcmBuffer_ + pcmBufferPos_, &pcm, sizeof(int16_t));
        pcmBufferPos_ += sizeof(int16_t);
    }

    int bytesPerFrame = 1024 * config_.channels * sizeof(int16_t);
    int totalEncoded = 0;

    while (pcmBufferPos_ >= bytesPerFrame) {
        AACENC_BufDesc inBuf = {};
        AACENC_InArgs inArgs = {};
        AACENC_OutArgs outArgs = {};

        inArgs.numInSamples = 1024 * config_.channels;

        INT inBufIds = IN_AUDIO_DATA;
        INT inBufSizes = bytesPerFrame;
        INT inBufElSizes = sizeof(int16_t);

        void* inPtr = pcmBuffer_;
        inBuf.numBufs = 1;
        inBuf.bufs = &inPtr;
        inBuf.bufferIdentifiers = &inBufIds;
        inBuf.bufSizes = &inBufSizes;
        inBuf.bufElSizes = &inBufElSizes;

        AACENC_BufDesc outBuf = {};
        INT outBufIds = OUT_BITSTREAM_DATA;
        INT outBufSizes = max_out_bytes - totalEncoded;
        INT outBufElSizes = sizeof(uint8_t);

        void* outPtr = enc_out + totalEncoded;
        outBuf.numBufs = 1;
        outBuf.bufs = &outPtr;
        outBuf.bufferIdentifiers = &outBufIds;
        outBuf.bufSizes = &outBufSizes;
        outBuf.bufElSizes = &outBufElSizes;

        AACENC_ERROR err = aacEncEncode(handle_, &inBuf, &outBuf, &inArgs, &outArgs);
        if (err != AACENC_OK) {
            fprintf(stderr, "[AacEncoder] Encode error: 0x%04x\n", err);
            break;
        }

        if (outArgs.numOutBytes > 0) {
            totalEncoded += outArgs.numOutBytes;
        }

        int consumed = outArgs.numInSamples * sizeof(int16_t);
        if (consumed > 0) {
            if (consumed < pcmBufferPos_) {
                memmove(pcmBuffer_, pcmBuffer_ + consumed, pcmBufferPos_ - consumed);
            }
            pcmBufferPos_ -= consumed;
        } else {
            break; // No progress
        }

        if (totalEncoded > max_out_bytes - 1024) break; // Avoid buffer overrun
    }

    return totalEncoded;
#else
    (void)pcm_in; (void)num_samples; (void)enc_out; (void)max_out_bytes;
    return 0;
#endif
}

int AacEncoder::flush(uint8_t* enc_out, int max_out_bytes) {
#ifdef HAVE_LIBFDK_AAC
    if (!handle_) return 0;

    AACENC_BufDesc outBuf = {};
    AACENC_InArgs inArgs = {};
    AACENC_OutArgs outArgs = {};

    inArgs.numInSamples = -1;

    INT outBufIds = OUT_BITSTREAM_DATA;
    INT outBufSizes = max_out_bytes;
    INT outBufElSizes = sizeof(uint8_t);

    void* outPtr = enc_out;
    outBuf.numBufs = 1;
    outBuf.bufs = &outPtr;
    outBuf.bufferIdentifiers = &outBufIds;
    outBuf.bufSizes = &outBufSizes;
    outBuf.bufElSizes = &outBufElSizes;

    aacEncEncode(handle_, nullptr, &outBuf, &inArgs, &outArgs);
    return outArgs.numOutBytes;
#else
    (void)enc_out; (void)max_out_bytes;
    return 0;
#endif
}

void AacEncoder::close() {
#ifdef HAVE_LIBFDK_AAC
    if (handle_) {
        aacEncClose(&handle_);
        handle_ = nullptr;
    }
    delete[] pcmBuffer_;
    pcmBuffer_ = nullptr;
    pcmBufferPos_ = 0;
#endif
}

} // namespace ore
