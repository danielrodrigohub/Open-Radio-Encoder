#pragma once
#include <JuceHeader.h>
#include <string>
#include <atomic>
#include <memory>
#include <vector>
#include <fstream>

#include "encoders/encoder_interface.h"

namespace ore {

class RecordingEngine {
public:
    RecordingEngine() = default;
    ~RecordingEngine();

    int startRecording(const std::string& path, const std::string& codec,
                       int sampleRate, int channels, int bitrate);
    void stopRecording();
    void feedAudio(const float* buffer, int frames, int channels);

    bool isRecording() const { return recording_.load(); }
    double kbytesWritten() const { return kbytesWritten_.load(); }
    const std::string& filePath() const { return filePath_; }
    const std::string& codec() const { return codec_; }
    double durationSecs() const { return durationSecs_.load(); }

private:
    int startEncodedRecording(const EncoderConfig& cfg);
    int startRawRecording(int sampleRate, int channels, int bitrate);

    std::atomic<bool> recording_{false};
    std::atomic<double> kbytesWritten_{0.0};
    std::atomic<double> durationSecs_{0.0};
    std::string filePath_;
    std::string codec_;

    // JUCE writer (WAV, FLAC, AIFF)
    std::unique_ptr<juce::AudioFormatWriter> writer_;

    // Butt-core encoder (MP3, Opus, MP2)
    std::unique_ptr<IEncoder> encoder_;
    std::vector<uint8_t> encBuffer_;
    std::unique_ptr<juce::FileOutputStream> rawStream_;
    EncoderConfig encConfig_;

    int sampleRate_ = 44100;
    int channels_ = 2;
};

} // namespace ore
