#include "RecordingEngine.h"
#include "encoders/lame_encoder.h"
#include "encoders/twolame_encoder.h"
#include "encoders/opus_encoder.h"
#include <cstdio>
#include <algorithm>

namespace ore {

RecordingEngine::~RecordingEngine() {
    stopRecording();
}

int RecordingEngine::startRecording(const std::string& path, const std::string& codec,
                                     int sampleRate, int channels, int bitrate) {
    if (recording_.load()) stopRecording();

    filePath_ = path;
    codec_ = codec;
    sampleRate_ = sampleRate;
    channels_ = channels;

    juce::File file(path);
    juce::Result result = file.create();
    if (result.failed()) {
        fprintf(stderr, "[RecordingEngine] Cannot create file: %s\n",
                result.getErrorMessage().toRawUTF8());
        return -1;
    }

    if (codec == "mp3" || codec == "mp2" || codec == "opus") {
        EncoderConfig cfg;
        cfg.codec = (codec == "mp3") ? CodecType::MP3
                  : (codec == "mp2") ? CodecType::MP2
                  : CodecType::Opus;
        cfg.samplerate = sampleRate;
        cfg.channels = channels;
        cfg.bitrate = bitrate;
        cfg.quality = 5;
        return startEncodedRecording(cfg);
    }

    return startRawRecording(sampleRate, channels, bitrate);
}

int RecordingEngine::startEncodedRecording(const EncoderConfig& cfg) {
    // Create appropriate encoder
    switch (cfg.codec) {
        case CodecType::MP3:  encoder_ = std::make_unique<LameEncoder>();  break;
        case CodecType::MP2:  encoder_ = std::make_unique<TwoLameEncoder>(); break;
        case CodecType::Opus: encoder_ = std::make_unique<OpusEncoder_>(); break;
        default: return -1;
    }

    if (encoder_->init(cfg) != 0) {
        fprintf(stderr, "[RecordingEngine] Failed to init encoder\n");
        encoder_.reset();
        return -1;
    }

    encConfig_ = cfg;
    encBuffer_.resize(static_cast<size_t>(cfg.samplerate * cfg.channels * sizeof(float) * 2));

    rawStream_ = std::make_unique<juce::FileOutputStream>(juce::File(filePath_));
    if (!rawStream_->openedOk()) {
        fprintf(stderr, "[RecordingEngine] Cannot open file\n");
        encoder_.reset();
        return -1;
    }

    kbytesWritten_.store(0.0);
    durationSecs_.store(0.0);
    recording_.store(true);

    fprintf(stdout, "[RecordingEngine] Started (encoder): %s (%s, %d Hz, %d ch, %d kbps)\n",
            filePath_.c_str(), encConfig_.codecName().c_str(), cfg.samplerate, cfg.channels, cfg.bitrate);
    return 0;
}

int RecordingEngine::startRawRecording(int sampleRate, int channels, int bitrate) {
    auto stream = std::make_unique<juce::FileOutputStream>(juce::File(filePath_));
    if (!stream->openedOk()) {
        fprintf(stderr, "[RecordingEngine] Cannot open file for writing\n");
        return -1;
    }

    std::unique_ptr<juce::OutputStream> outStream = std::move(stream);

    auto opts = juce::AudioFormatWriterOptions{}
        .withSampleRate(static_cast<double>(sampleRate))
        .withNumChannels(channels)
        .withBitsPerSample(std::max(bitrate, 16))
        .withQualityOptionIndex(0);

    if (codec_ == "flac") {
        juce::FlacAudioFormat fmt;
        writer_ = fmt.createWriterFor(outStream, opts);
    } else if (codec_ == "aiff") {
        juce::AiffAudioFormat fmt;
        writer_ = fmt.createWriterFor(outStream, opts);
    } else {
        juce::WavAudioFormat fmt;
        writer_ = fmt.createWriterFor(outStream, opts);
    }

    if (!writer_) {
        fprintf(stderr, "[RecordingEngine] Failed to create audio writer\n");
        return -1;
    }

    kbytesWritten_.store(0.0);
    durationSecs_.store(0.0);
    recording_.store(true);

    fprintf(stdout, "[RecordingEngine] Started (raw): %s (%s, %d Hz, %d ch)\n",
            filePath_.c_str(), codec_.c_str(), sampleRate, channels);
    return 0;
}

void RecordingEngine::stopRecording() {
    if (!recording_.load()) return;
    recording_.store(false);

    if (encoder_) {
        // Flush remaining encoded data
        std::vector<uint8_t> flushBuf(4096);
        int flushed = encoder_->flush(flushBuf.data(), static_cast<int>(flushBuf.size()));
        if (flushed > 0 && rawStream_) {
            rawStream_->write(flushBuf.data(), flushed);
        }
        encoder_->close();
        encoder_.reset();
    }

    if (writer_) {
        writer_->flush();
        writer_.reset();
    }

    if (rawStream_) {
        rawStream_->flush();
        rawStream_.reset();
    }

    encBuffer_.clear();

    fprintf(stdout, "[RecordingEngine] Stopped: %s (%.1f KB)\n",
            filePath_.c_str(), kbytesWritten_.load());
}

void RecordingEngine::feedAudio(const float* buffer, int frames, int channels) {
    if (!recording_.load()) return;

    if (encoder_ && rawStream_) {
        int encBytes = encoder_->encode(buffer, frames,
                                         encBuffer_.data(),
                                         static_cast<int>(encBuffer_.size()));
        if (encBytes > 0) {
            rawStream_->write(encBuffer_.data(), encBytes);
            double kb = static_cast<double>(encBytes) / 1024.0;
            kbytesWritten_.store(kbytesWritten_.load() + kb);
        }
    } else if (writer_) {
        std::vector<const float*> chPtrs(static_cast<size_t>(channels));
        std::vector<std::vector<float>> deint(static_cast<size_t>(channels));

        if (channels == 1) {
            chPtrs[0] = buffer;
        } else {
            for (int ch = 0; ch < channels; ch++) {
                deint[static_cast<size_t>(ch)].resize(static_cast<size_t>(frames));
                for (int i = 0; i < frames; i++) {
                    float s = buffer[i * channels + ch];
                    s = std::clamp(s, -1.0f, 1.0f);
                    deint[static_cast<size_t>(ch)][static_cast<size_t>(i)] = s;
                }
                chPtrs[static_cast<size_t>(ch)] = deint[static_cast<size_t>(ch)].data();
            }
        }

        juce::AudioSampleBuffer fb(const_cast<float* const*>(chPtrs.data()), channels, frames);
        if (!writer_->writeFromAudioSampleBuffer(fb, 0, frames)) {
            fprintf(stderr, "[RecordingEngine] Write error\n");
            stopRecording();
            return;
        }

        double kb = static_cast<double>(frames * channels * sizeof(float)) / 1024.0;
        kbytesWritten_.store(kbytesWritten_.load() + kb);
    }

    durationSecs_.store(durationSecs_.load() + static_cast<double>(frames) / sampleRate_);
}

} // namespace ore
