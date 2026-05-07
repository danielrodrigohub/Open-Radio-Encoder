// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Station Connection
// One instance per streaming server. Owns its own thread, encoder,
// ring buffer, and protocol handler.
//
// This replaces BUTT's single snd_stream_thread() with a per-station
// thread that can independently encode and send audio data.
// ═══════════════════════════════════════════════════════════════════════
#pragma once

#include <JuceHeader.h>
#include "BroadcastDistributor.h"
#include "encoders/encoder_interface.h"
#include "protocols/icecast_client.h"
#include "protocols/shoutcast_client.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef HAVE_SAMPLERATE
#include <samplerate.h>
#endif

namespace ore {

class BroadcastDistributor;
class DSPEffects;
class VST3HostProcessor;
class RecordingEngine;

/// VU meter data for UI display
struct VUMeterData;

class StationConnection {
public:
    StationConnection();
    ~StationConnection();

    /// Configure this station (must be disconnected).
    void configure(const StationConfig& config);

    /// Get the configuration.
    const StationConfig& config() const { return config_; }

    /// Launch the streaming thread and connect to the server.
    void connect();

    /// Stop the streaming thread and disconnect.
    void disconnect();

    /// Is this station currently connected and streaming?
    bool isConnected() const { return state_.load() == StationState::Connected; }

    /// Feed audio data into this station's ring buffer.
    /// Called from the mixer thread (via BroadcastDistributor::distribute).
    void feedAudio(const float* buffer, int frames, int channels, int sampleRate);

    /// Get a snapshot of the current status.
    StationStatus status() const;

    /// Update the song name (metadata push to server).
    void updateSongName(const std::string& songName);

private:
    /// The streaming thread function.
    /// This is the per-station equivalent of BUTT's snd_stream_thread().
    void streamingLoop();

    /// Create the appropriate encoder based on config.
    std::unique_ptr<IEncoder> createEncoder(CodecType type);

    /// Connect to the server (Icecast or Shoutcast protocol).
    int connectToServer();

    /// Send encoded data to the server.
    int sendToServer(const uint8_t* data, int len);

    /// Disconnect from the server.
    void disconnectFromServer();

    StationConfig config_;
    std::atomic<StationState> state_{StationState::Disconnected};

    // Streaming thread
    std::thread thread_;
    std::atomic<bool> shouldRun_{false};

    // Ring buffer (SPSC: mixer thread writes, station thread reads)
    std::vector<float> ringBuffer_;
    std::atomic<size_t> rbWritePos_{0};
    std::atomic<size_t> rbReadPos_{0};
    size_t rbCapacity_ = 0;

    // Condition variable to wake the streaming thread
    std::mutex cvMutex_;
    std::condition_variable cv_;

    // Encoder
    std::unique_ptr<IEncoder> encoder_;

    // Protocol clients (one or the other, depending on serverType)
    std::unique_ptr<IcecastClient> icecastClient_;
    std::unique_ptr<ShoutcastClient> shoutcastClient_;

    // Encoded output buffer
    std::vector<uint8_t> encodeBuffer_;

    // Metadata queue (must be sent from streaming thread for thread safety)
    std::mutex metadataMutex_;
    std::string pendingSongName_;
    bool hasPendingMetadata_ = false;

    void sendPendingMetadata();

    // Resampling
    std::atomic<int> inputSampleRate_{0};
#ifdef HAVE_SAMPLERATE
    SRC_STATE* resampler_ = nullptr;
    std::vector<float> resampleBuffer_;
#endif

    // Statistics
    std::atomic<double> kbytesSent_{0.0};
    std::atomic<double> streamStartTime_{0.0};
    std::atomic<int> listenerCount_{0};
    std::string errorMessage_;
};

} // namespace ore
