// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Audio Pipeline Implementation
// Refactored from BUTT 1.46.0 port_audio.cpp
// ═══════════════════════════════════════════════════════════════════════
#include "AudioPipeline.h"
#include "BroadcastDistributor.h"

#ifdef HAVE_PORTAUDIO
#include <portaudio.h>
#endif

#include <algorithm>
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
}

AudioPipeline::~AudioPipeline() {
    stop();
}

// ─────────────────────────────────────────────
// Initialize PortAudio
// ─────────────────────────────────────────────
int AudioPipeline::initialize(const PipelineConfig& config) {
    config_ = config;

#ifdef HAVE_PORTAUDIO
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        fprintf(stderr, "[AudioPipeline] Pa_Initialize failed: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    // Allocate ring buffer: 16x buffer size for safety
    ringCapacity_ = config_.bufferSize * config_.channels * 16;
    ringBuffer_.resize(ringCapacity_, 0.0f);
    ringWritePos_.store(0);
    ringReadPos_.store(0);

    // Allocate processing buffer
    processedBuffer_.resize(config_.bufferSize * config_.channels, 0.0f);

    // Open PortAudio stream
    PaStreamParameters params;
    PaDeviceIndex devId = (config_.deviceIndex >= 0)
        ? config_.deviceIndex
        : Pa_GetDefaultInputDevice();

    const PaDeviceInfo* devInfo = Pa_GetDeviceInfo(devId);
    if (!devInfo) {
        fprintf(stderr, "[AudioPipeline] Invalid device ID: %d\n", devId);
        return -1;
    }

    params.device = devId;
    params.channelCount = devInfo->maxInputChannels;
    params.sampleFormat = paFloat32;
    params.suggestedLatency = devInfo->defaultHighInputLatency;
    params.hostApiSpecificStreamInfo = nullptr;

    err = Pa_OpenStream(
        reinterpret_cast<PaStream**>(&paStream_),
        &params, nullptr,
        config_.sampleRate,
        config_.bufferSize,
        paNoFlag,
        &AudioPipeline::paCallback,
        this
    );

    if (err != paNoError) {
        fprintf(stderr, "[AudioPipeline] Pa_OpenStream failed: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    fprintf(stdout, "[AudioPipeline] Opened device '%s' @ %d Hz, %d ch, %d frames\n",
            devInfo->name, config_.sampleRate, config_.channels, config_.bufferSize);

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
    running_.store(true);

#ifdef HAVE_PORTAUDIO
    Pa_StartStream(reinterpret_cast<PaStream*>(paStream_));
#endif

    mixerThread_ = std::thread(&AudioPipeline::mixerLoop, this);

    fprintf(stdout, "[AudioPipeline] Started\n");
}

void AudioPipeline::stop() {
    if (!running_.load()) return;
    running_.store(false);

    if (mixerThread_.joinable())
        mixerThread_.join();

#ifdef HAVE_PORTAUDIO
    if (paStream_) {
        Pa_StopStream(reinterpret_cast<PaStream*>(paStream_));
        Pa_CloseStream(reinterpret_cast<PaStream*>(paStream_));
        paStream_ = nullptr;
    }
    Pa_Terminate();
#endif

    fprintf(stdout, "[AudioPipeline] Stopped\n");
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

    while (running_.load()) {
        // Wait for enough data in the ring buffer
        size_t wp = ringWritePos_.load(std::memory_order_acquire);
        size_t rp = ringReadPos_.load(std::memory_order_relaxed);
        size_t available = ringAvailableRead(wp, rp, ringCapacity_);

        if (available < static_cast<size_t>(frameSize)) {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
            continue;
        }

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
        // TODO: Call DSPEffects::processSamples() here
        // This is where BUTT's streaming_dsp->processSamples(stream_buf) goes

        // ── Stage 3: VST3 Plugin Chain ──
        // TODO: Call AudioProcessorGraph::processBlock() here
        // This is the NEW VST3 hosting step

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
                                     config_.channels);
        }
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
    Pa_Initialize();
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
#endif
    return devices;
}

} // namespace ore
