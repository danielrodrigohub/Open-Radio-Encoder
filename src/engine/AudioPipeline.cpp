// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Audio Pipeline Implementation
// Refactored from BUTT 1.46.0 port_audio.cpp
// ═══════════════════════════════════════════════════════════════════════
#include "AudioPipeline.h"
#include "BroadcastDistributor.h"
#include "VST3HostProcessor.h"
#include "RecordingEngine.h"
#include "dsp/dsp_effects.h"

#ifdef HAVE_PORTAUDIO
#include <portaudio.h>
#endif

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace ore {

// ─────────────────────────────────────────────
// Ring buffer helpers (lock-free SPSC)
// ─────────────────────────────────────────────
static inline size_t ringAvailableWrite(size_t writePos, size_t readPos, size_t cap) {
    return (readPos + cap - writePos - 1) % cap;
}
static inline size_t ringAvailableRead(size_t writePos, size_t readPos, size_t cap) {
    return (writePos + cap - readPos) % cap;
}

// ─────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────
AudioPipeline::AudioPipeline() {
    distributor_ = std::make_unique<BroadcastDistributor>();
    recordingEngine_ = std::make_unique<RecordingEngine>();

#ifdef HAVE_PORTAUDIO
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "[AudioPipeline] Pa_Initialize failed: %s\n", Pa_GetErrorText(err));
    }
#endif
}

AudioPipeline::~AudioPipeline() {
    stop();
#ifdef HAVE_PORTAUDIO
    Pa_Terminate();
#endif
}

// ─────────────────────────────────────────────
// Initialize PortAudio
// ─────────────────────────────────────────────
int AudioPipeline::initialize(const PipelineConfig& config) {
#ifdef HAVE_PORTAUDIO
    // Open PortAudio stream
    PaStreamParameters params;
    PaDeviceIndex devId = (config.deviceIndex >= 0)
        ? config.deviceIndex
        : Pa_GetDefaultInputDevice();

    if (devId == paNoDevice) {
        fprintf(stderr, "[AudioPipeline] No default input device found\n");
        return -1;
    }

    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(devId);
    if (!devInfo) {
        fprintf(stderr, "[AudioPipeline] Invalid device ID: %d\n", devId);
        return -1;
    }

    const PaHostApiInfo* hostInfo = Pa_GetHostApiInfo(devInfo->hostApi);
    fprintf(stdout, "[AudioPipeline] Initializing device %d: %s\n", devId, devInfo->name);
    fprintf(stdout, "               Host API: %s\n", hostInfo ? hostInfo->name : "Unknown");
    fprintf(stdout, "               Max Input Channels: %d\n", devInfo->maxInputChannels);
    fprintf(stdout, "               Default Sample Rate: %.1f Hz\n", devInfo->defaultSampleRate);
    fprintf(stdout, "               Requested: %d ch @ %d Hz\n", config.channels, config.sampleRate);

    // Check if device supports input
    if (devInfo->maxInputChannels <= 0) {
        fprintf(stderr, "[AudioPipeline] Device '%s' has no input channels\n", devInfo->name);
        return -1;
    }

    // Clamp to available channels
    int actualChannels = std::min(config.channels, devInfo->maxInputChannels);
    if (actualChannels < 1) actualChannels = 1;
    
    params.device = devId;
    params.channelCount = actualChannels;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = devInfo->defaultHighInputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    // Verify format
    PaError formatErr = Pa_IsFormatSupported(&params, nullptr, config.sampleRate);
    if (formatErr != paFormatIsSupported) {
        fprintf(stderr, "[AudioPipeline] Format check failed: %s\n", Pa_GetErrorText(formatErr));
    }

    // Try to open the stream
    PaError err = Pa_OpenStream(
        reinterpret_cast<PaStream**>(&paStream_),
        &params, nullptr,
        config.sampleRate,
        config.bufferSize,
        paNoFlag,
        &AudioPipeline::paCallback,
        this
    );

    // Fallback: If 2 channels failed, try 1 channel
    if (err == paInvalidChannelCount && actualChannels > 1) {
        fprintf(stdout, "[AudioPipeline] 2-channel open failed, retrying with mono...\n");
        actualChannels = 1;
        params.channelCount = 1;
        err = Pa_OpenStream(
            reinterpret_cast<PaStream**>(&paStream_),
            &params, nullptr,
            config.sampleRate,
            config.bufferSize,
            paNoFlag,
            &AudioPipeline::paCallback,
            this
        );
    }

    if (err != paNoError) {
        fprintf(stderr, "[AudioPipeline] Pa_OpenStream failed: %s (ch=%d, rate=%d)\n", 
                Pa_GetErrorText(err), actualChannels, config.sampleRate);
        return -1;
    }

    // Success! Update internal config
    config_ = config;
    config_.channels = actualChannels;
    
    // EXPLICIT LOGGING FOR RESAMPLING VERIFICATION
    fprintf(stdout, "[AudioPipeline] SYSTEM SAMPLE RATE SET TO: %d Hz\n", config_.sampleRate);

    // Ensure buffers match the actual channel count
    ringCapacity_ = config_.bufferSize * config_.channels * 16;
    ringBuffer_.assign(ringCapacity_, 0.0f);
    ringWritePos_.store(0);
    ringReadPos_.store(0);
    processedBuffer_.assign(config_.bufferSize * config_.channels, 0.0f);

    fprintf(stdout, "[AudioPipeline] Successfully opened device '%s' @ %d Hz, %d ch, %d frames\n",
            devInfo->name, config_.sampleRate, config_.channels, config_.bufferSize);

    // Initialize DSP effects (EQ + Compressor)
    dsp_ = std::make_unique<DSPEffects>(
        static_cast<uint32_t>(config_.bufferSize),
        static_cast<uint8_t>(config_.channels),
        static_cast<uint32_t>(config_.sampleRate));

    // Initialize VST3 host processor
    vst3Host_ = std::make_unique<VST3HostProcessor>();
    vst3Host_->initialize(config_.sampleRate, config_.bufferSize, config_.channels);

    return 0;
#else
    fprintf(stderr, "[AudioPipeline] Built without PortAudio\n");
    return -1;
#endif
}

// ─────────────────────────────────────────────
// Start / Stop
// ─────────────────────────────────────────────
void AudioPipeline::start() {
    if (running_.load()) return;
    
#ifdef HAVE_PORTAUDIO
    if (paStream_) {
        running_.store(true);
        Pa_StartStream(reinterpret_cast<PaStream*>(paStream_));
        mixerThread_ = std::thread(&AudioPipeline::mixerLoop, this);
        fprintf(stdout, "[AudioPipeline] Started\n");
    } else {
        fprintf(stderr, "[AudioPipeline] Cannot start: Stream not initialized\n");
    }
#endif
}

void AudioPipeline::stop() {
    running_.store(false);

    if (mixerThread_.joinable())
        mixerThread_.join();

#ifdef HAVE_PORTAUDIO
    if (paStream_) {
        Pa_StopStream(reinterpret_cast<PaStream*>(paStream_));
        Pa_CloseStream(reinterpret_cast<PaStream*>(paStream_));
        paStream_ = nullptr;
    }
#endif

    fprintf(stdout, "[AudioPipeline] Stopped\n");
}

int AudioPipeline::restart(const PipelineConfig& config) {
    fprintf(stdout, "[AudioPipeline] Restarting with new config...\n");
    
    // Save current config for recovery
    PipelineConfig oldConfig = config_;
    
    stop();
    
    int rc = initialize(config);
    if (rc == 0) {
        start();
    } else {
        fprintf(stderr, "[AudioPipeline] Re-initialization failed, attempting to restore original config\n");
        // Try to restore previous known good configuration
        rc = initialize(oldConfig);
        if (rc == 0) {
            start();
        } else {
            fprintf(stderr, "[AudioPipeline] FATAL: Could not even restore original config\n");
        }
    }
    return rc;
}

// ─────────────────────────────────────────────
// PortAudio Callback (Real-time thread)
//
// This replaces BUTT's snd_callback() function.
// We only do channel mapping + ring buffer write here.
// All DSP happens in the mixer thread (non-real-time safe).
// ─────────────────────────────────────────────
int AudioPipeline::paCallback(const void* input, void* /*output*/,
                               unsigned long frameCount,
                               const PaStreamCallbackTimeInfo* /*timeInfo*/,
                               PaStreamCallbackFlags /*statusFlags*/,
                               void* userData) {
    auto* self = static_cast<AudioPipeline*>(userData);
    const auto* pcmInput = static_cast<const float*>(input);
    if (!pcmInput) return 0; // paContinue

    const int channels = self->config_.channels;
    const size_t samplesToWrite = frameCount * channels;

    // Write to ring buffer (non-blocking)
    size_t wp = self->ringWritePos_.load(std::memory_order_relaxed);
    size_t rp = self->ringReadPos_.load(std::memory_order_acquire);

    if (ringAvailableWrite(wp, rp, self->ringCapacity_) < samplesToWrite) {
        // Ring buffer overflow — drop this frame (better than blocking in RT thread)
        return 0; // paContinue
    }

    for (size_t i = 0; i < samplesToWrite; i++) {
        self->ringBuffer_[(wp + i) % self->ringCapacity_] = pcmInput[i];
    }
    self->ringWritePos_.store((wp + samplesToWrite) % self->ringCapacity_,
                               std::memory_order_release);

    return 0; // paContinue
}

// ─────────────────────────────────────────────
// Mixer Thread
//
// This replaces BUTT's snd_mixer_thread() function.
// Runs in a dedicated thread, reads the ring buffer,
// applies DSP, and distributes to all station threads.
// ─────────────────────────────────────────────
void AudioPipeline::mixerLoop() {
    const int frameSize = config_.bufferSize * config_.channels;
    const double blockTimeSecs = static_cast<double>(config_.bufferSize) / config_.sampleRate;

    while (running_.load()) {
        // Wait for enough data in the ring buffer
        size_t wp = ringWritePos_.load(std::memory_order_acquire);
        size_t rp = ringReadPos_.load(std::memory_order_relaxed);
        size_t available = ringAvailableRead(wp, rp, ringCapacity_);

        if (available < static_cast<size_t>(frameSize)) {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            continue;
        }

        // ── Start Timing ──
        auto startTime = std::chrono::high_resolution_clock::now();

        // Read one frame from ring buffer
        for (int i = 0; i < frameSize; i++) {
            processedBuffer_[i] = ringBuffer_[(rp + i) % ringCapacity_];
        }
        ringReadPos_.store((rp + frameSize) % ringCapacity_,
                            std::memory_order_release);

        // ── Stage 1: Master Gain ──
        float gain = masterGain_.load(std::memory_order_relaxed);
        if (gain != 1.0f) {
            for (int i = 0; i < frameSize; i++) {
                processedBuffer_[i] *= gain;
            }
        }

        // ── Stage 2: Internal DSP (EQ + Compressor) ──
        if (dsp_ && dsp_->hasToProcessSamples()) {
            dsp_->processSamples(processedBuffer_.data(), config_.bufferSize);
        }

        // ── Stage 3: VST3 Plugin Chain ──
        if (vst3Host_ && !vst3Host_->plugins().empty()) {
            vst3Host_->processBlock(processedBuffer_.data(),
                                    config_.bufferSize,
                                    config_.channels);
        }

        // ── Stage 4: Clamp output ──
        for (int i = 0; i < frameSize; i++) {
            processedBuffer_[i] = std::clamp(processedBuffer_[i], -1.0f, 1.0f);
        }

        // ── Stage 5: VU Meters ──
        updateVUMeters(processedBuffer_.data(), config_.bufferSize, config_.channels);

        // ── Stage 6: Distribute to all station threads ──
        if (distributor_) {
            distributor_->distribute(processedBuffer_.data(),
                                     config_.bufferSize,
                                     config_.channels,
                                     config_.sampleRate);
        }

        // ── Stage 7: Recording feed ──
        if (recordingEngine_ && recordingEngine_->isRecording()) {
            recordingEngine_->feedAudio(processedBuffer_.data(),
                                        config_.bufferSize,
                                        config_.channels);
        }

        // ── End Timing & Calculate CPU Load ──
        auto endTime = std::chrono::high_resolution_clock::now();
        double elapsedSecs = std::chrono::duration<double>(endTime - startTime).count();
        float blockCpu = static_cast<float>(elapsedSecs / blockTimeSecs);
        
        // Exponential smoothing (EMA) for a stable but reactive indicator
        float currentCpu = cpuUsage_.load(std::memory_order_relaxed);
        cpuUsage_.store(currentCpu * 0.9f + blockCpu * 0.1f, std::memory_order_relaxed);
    }
}

// ─────────────────────────────────────────────
// VU Meter Calculation
// ─────────────────────────────────────────────
void AudioPipeline::updateVUMeters(const float* buffer, int numFrames, int channels) {
    float maxL = 0.0f, maxR = 0.0f;
    float sumL = 0.0f, sumR = 0.0f;

    for (int i = 0; i < numFrames; i++) {
        float l = buffer[i * channels];
        float r = (channels == 2) ? buffer[i * channels + 1] : l;

        float absL = std::fabs(l);
        float absR = std::fabs(r);

        maxL = std::max(maxL, absL);
        maxR = std::max(maxR, absR);
        sumL += l * l;
        sumR += r * r;
    }

    // Convert to dB
    auto toDB = [](float linear) -> float {
        if (linear < 1e-10f) return -100.0f;
        return 20.0f * std::log10(linear);
    };

    vuData_.peakL.store(toDB(maxL), std::memory_order_relaxed);
    vuData_.peakR.store(toDB(maxR), std::memory_order_relaxed);
    vuData_.rmsL.store(toDB(std::sqrt(sumL / numFrames)), std::memory_order_relaxed);
    vuData_.rmsR.store(toDB(std::sqrt(sumR / numFrames)), std::memory_order_relaxed);
}

// ─────────────────────────────────────────────
// Device Enumeration
// ─────────────────────────────────────────────
std::vector<AudioPipeline::DeviceInfo> AudioPipeline::getAvailableDevices() {
    std::vector<DeviceInfo> devices;
#ifdef HAVE_PORTAUDIO
    if (Pa_Initialize() == paNoError) {
        int count = Pa_GetDeviceCount();
        for (int i = 0; i < count; i++) {
            const PaDeviceInfo* info = Pa_GetDeviceInfo(i);
            if (info && info->maxInputChannels > 0) {
                devices.push_back({
                    i,
                    info->name ? info->name : "Unknown",
                    info->maxInputChannels,
                    static_cast<int>(info->defaultSampleRate)
                });
            }
        }
        Pa_Terminate();
    }
#endif
    return devices;
}

} // namespace ore
