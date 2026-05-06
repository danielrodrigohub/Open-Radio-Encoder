// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Audio Pipeline
// Master orchestrator: PortAudio → DSP → VST3 → Distributor
//
// This is the refactored heart of BUTT's port_audio.cpp mixer thread,
// redesigned as a clean C++ class that:
// 1. Captures audio from PortAudio
// 2. Passes it through internal DSP (EQ + Compressor)
// 3. Sends it through a VST3 plugin chain (AudioProcessorGraph)
// 4. Distributes the processed buffer to N station threads
// 5. Feeds VU meters with peak/RMS data
// ═══════════════════════════════════════════════════════════════════════
#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#ifdef HAVE_PORTAUDIO
#include <portaudio.h>
#else
struct PaStreamCallbackTimeInfo {};
using PaStreamCallbackFlags = unsigned long;
#endif

namespace ore {

class BroadcastDistributor;

/// VU meter data for UI display
struct VUMeterData {
    std::atomic<float> peakL{0.0f};
    std::atomic<float> peakR{0.0f};
    std::atomic<float> rmsL{0.0f};
    std::atomic<float> rmsR{0.0f};
};

/// Audio pipeline configuration
struct PipelineConfig {
    int sampleRate   = 44100;
    int channels     = 2;
    int bufferSize   = 1024;  // frames per buffer
    int deviceIndex  = -1;    // -1 = default device
    int device2Index = -1;    // -1 = no secondary device
};

/// The master audio pipeline that captures, processes, and distributes audio.
///
/// Thread model:
///   - PortAudio callback (real-time thread) → ring buffer
///   - Mixer thread → reads ring buffer, applies DSP, distributes
///   - N station threads → owned by BroadcastDistributor
///   - Recording thread → separate
class AudioPipeline {
public:
    AudioPipeline();
    ~AudioPipeline();

    /// Initialize PortAudio and open the configured device(s).
    /// Returns 0 on success.
    int initialize(const PipelineConfig& config);

    /// Start the mixer thread (begins capturing and processing).
    void start();

    /// Stop the mixer thread and close PortAudio.
    void stop();

    /// Get the VU meter data (thread-safe atomic reads).
    const VUMeterData& vuMeterData() const { return vuData_; }

    /// Get the broadcast distributor for managing station connections.
    BroadcastDistributor* distributor() { return distributor_.get(); }

    /// Set master gain (linear, 1.0 = unity).
    void setMasterGain(float gain) { masterGain_.store(gain); }

    /// Check if the pipeline is running.
    bool isRunning() const { return running_.load(); }

    /// Get list of available audio devices.
    struct DeviceInfo {
        int id;
        std::string name;
        int maxInputChannels;
        int defaultSampleRate;
    };
    static std::vector<DeviceInfo> getAvailableDevices();

private:
    /// PortAudio callback (static, called from real-time thread).
    static int paCallback(const void* input, void* output,
                          unsigned long frameCount,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void* userData);

    /// Mixer thread function.
    void mixerLoop();

    /// Calculate VU meter values from a buffer.
    void updateVUMeters(const float* buffer, int numFrames, int channels);

    PipelineConfig config_;
    std::atomic<bool> running_{false};
    std::atomic<float> masterGain_{1.0f};

    // Ring buffer for PA callback → mixer thread (lock-free SPSC)
    std::vector<float> ringBuffer_;
    std::atomic<size_t> ringWritePos_{0};
    std::atomic<size_t> ringReadPos_{0};
    size_t ringCapacity_ = 0;

    // Mixer thread
    std::thread mixerThread_;

    // Processed output buffers
    std::vector<float> processedBuffer_;

    // VU meter data
    VUMeterData vuData_;

    // Sub-systems
    std::unique_ptr<BroadcastDistributor> distributor_;

    // PortAudio stream handle (void* to avoid portaudio.h in header)
    void* paStream_ = nullptr;
};

} // namespace ore
