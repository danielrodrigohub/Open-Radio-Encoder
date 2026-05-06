// Recording Engine — stub
#pragma once
#include <string>
#include <atomic>
namespace ore {
class RecordingEngine {
public:
    int startRecording(const std::string& path, const std::string& codec, int sampleRate, int channels, int bitrate);
    void stopRecording();
    void feedAudio(const float* buffer, int frames, int channels);
    bool isRecording() const { return recording_.load(); }
    double kbytesWritten() const { return kbytesWritten_.load(); }
    const std::string& filePath() const { return filePath_; }
private:
    std::atomic<bool> recording_{false};
    std::atomic<double> kbytesWritten_{0.0};
    std::string filePath_;
};
} // namespace ore
