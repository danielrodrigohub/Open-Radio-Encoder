// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Station Connection Implementation
//
// This is the per-station streaming thread, the multi-server equivalent
// of BUTT's snd_stream_thread() in port_audio.cpp (line 785-915).
//
// Key difference from BUTT:
//   BUTT: ONE thread, ONE encoder, ONE socket, ONE server
//   HERE: N threads, N encoders, N sockets, N servers
// ═══════════════════════════════════════════════════════════════════════
#include "StationConnection.h"

#include "encoders/lame_encoder.h"
#include "encoders/twolame_encoder.h"
#include "encoders/aac_encoder.h"
#include "encoders/opus_encoder.h"
#include "encoders/vorbis_encoder.h"
#include "encoders/flac_encoder.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>

namespace ore {

// ─────────────────────────────────────────────
// Ring buffer helpers
// ─────────────────────────────────────────────
static size_t rbAvailRead(size_t wr, size_t rd, size_t cap) {
    return (wr + cap - rd) % cap;
}
static size_t rbAvailWrite(size_t wr, size_t rd, size_t cap) {
    return (rd + cap - wr - 1) % cap;
}

// ─────────────────────────────────────────────
StationConnection::StationConnection() = default;

StationConnection::~StationConnection() {
    disconnect();
}

void StationConnection::configure(const StationConfig& config) {
    config_ = config;

    // Size ring buffer for ~2 seconds of audio at the configured rate
    int framesPerSecond = config_.encoder.samplerate;
    rbCapacity_ = framesPerSecond * config_.encoder.channels * 2;
    ringBuffer_.resize(rbCapacity_, 0.0f);
    rbWritePos_.store(0);
    rbReadPos_.store(0);

    // Size encode output buffer (generous: 2x the PCM size should be more than enough)
    encodeBuffer_.resize(rbCapacity_ * sizeof(float) * 2);
}

// ─────────────────────────────────────────────
// Feed Audio (called from mixer thread)
//
// This is a non-blocking write to the station's ring buffer.
// If the ring buffer is full (station thread is stalling),
// we drop the audio frame. This prevents the mixer thread
// from blocking due to one slow station.
// ─────────────────────────────────────────────
void StationConnection::feedAudio(const float* buffer, int frames, int channels) {
    size_t samplesToWrite = frames * channels;
    size_t wr = rbWritePos_.load(std::memory_order_relaxed);
    size_t rd = rbReadPos_.load(std::memory_order_acquire);

    if (rbAvailWrite(wr, rd, rbCapacity_) < samplesToWrite) {
        return; // Drop frame rather than block
    }

    for (size_t i = 0; i < samplesToWrite; i++) {
        ringBuffer_[(wr + i) % rbCapacity_] = buffer[i];
    }
    rbWritePos_.store((wr + samplesToWrite) % rbCapacity_,
                       std::memory_order_release);

    // Wake up the streaming thread
    cv_.notify_one();
}

// ─────────────────────────────────────────────
// Connect — Launch the streaming thread
// ─────────────────────────────────────────────
void StationConnection::connect() {
    if (shouldRun_.load()) return;

    state_.store(StationState::Connecting);
    shouldRun_.store(true);
    kbytesSent_.store(0.0);

    // Create encoder for this station's codec
    encoder_ = createEncoder(config_.encoder.codec);
    if (!encoder_ || encoder_->init(config_.encoder) != 0) {
        state_.store(StationState::Error);
        errorMessage_ = "Failed to initialize encoder";
        shouldRun_.store(false);
        return;
    }

    // Launch the streaming thread
    thread_ = std::thread(&StationConnection::streamingLoop, this);

    fprintf(stdout, "[Station %d] Connecting to %s:%d%s (%s)\n",
            config_.id, config_.address.c_str(), config_.port,
            config_.mountPoint.c_str(),
            config_.encoder.codecName().c_str());
}

// ─────────────────────────────────────────────
// Disconnect — Stop the streaming thread
// ─────────────────────────────────────────────
void StationConnection::disconnect() {
    if (!shouldRun_.load()) return;

    shouldRun_.store(false);
    cv_.notify_all(); // Wake the thread so it can exit

    if (thread_.joinable()) {
        thread_.join();
    }

    disconnectFromServer();

    if (encoder_) {
        encoder_->close();
        encoder_.reset();
    }

    state_.store(StationState::Disconnected);
    fprintf(stdout, "[Station %d] Disconnected\n", config_.id);
}

// ─────────────────────────────────────────────
// Streaming Loop — Per-station thread
//
// This is the direct equivalent of BUTT's snd_stream_thread()
// (port_audio.cpp line 785), but operates independently per station.
//
// Flow:
//   1. Connect to server (Icecast/Shoutcast protocol)
//   2. Loop:
//      a. Wait for audio in ring buffer
//      b. Read a chunk from ring buffer
//      c. Encode (MP3/MP2/AAC/Opus/Vorbis/FLAC)
//      d. Send encoded bytes to server
//   3. On error: reconnect with backoff
// ─────────────────────────────────────────────
void StationConnection::streamingLoop() {
    // ── Phase 1: Connect to server ──
    int connectResult = connectToServer();
    if (connectResult != 0) {
        state_.store(StationState::Error);
        errorMessage_ = "Connection failed";
        return;
    }

    state_.store(StationState::Connected);
    auto startTime = std::chrono::steady_clock::now();

    // Frame size for encoding (same as BUTT's framepacket_size)
    const int frameSize = 1024;  // frames per encoding chunk
    const int channels = config_.encoder.channels;
    const int samplesPerChunk = frameSize * channels;
    const int bytesToRead = samplesPerChunk * sizeof(float) / sizeof(float);

    std::vector<float> audioBuf(samplesPerChunk);

    // ── Phase 2: Encode + Send loop ──
    while (shouldRun_.load()) {
        // Wait for data
        {
            std::unique_lock<std::mutex> lock(cvMutex_);
            cv_.wait_for(lock, std::chrono::milliseconds(100), [&]() {
                size_t wr = rbWritePos_.load(std::memory_order_acquire);
                size_t rd = rbReadPos_.load(std::memory_order_relaxed);
                return rbAvailRead(wr, rd, rbCapacity_) >= static_cast<size_t>(samplesPerChunk)
                       || !shouldRun_.load();
            });
        }

        if (!shouldRun_.load()) break;

        // Read and encode all available chunks
        while (true) {
            size_t wr = rbWritePos_.load(std::memory_order_acquire);
            size_t rd = rbReadPos_.load(std::memory_order_relaxed);
            size_t avail = rbAvailRead(wr, rd, rbCapacity_);

            if (avail < static_cast<size_t>(samplesPerChunk)) break;

            // Read from ring buffer
            for (int i = 0; i < samplesPerChunk; i++) {
                audioBuf[i] = ringBuffer_[(rd + i) % rbCapacity_];
            }
            rbReadPos_.store((rd + samplesPerChunk) % rbCapacity_,
                              std::memory_order_release);

            // Encode
            int encBytes = encoder_->encode(
                audioBuf.data(), frameSize,
                encodeBuffer_.data(),
                static_cast<int>(encodeBuffer_.size())
            );

            if (encBytes <= 0) continue;

            // Send to server
            int sent = sendToServer(encodeBuffer_.data(), encBytes);
            if (sent < 0) {
                // Connection lost — try to reconnect
                state_.store(StationState::Reconnecting);
                disconnectFromServer();

                // Backoff retry
                for (int retry = 0; retry < 10 && shouldRun_.load(); retry++) {
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                    if (connectToServer() == 0) {
                        state_.store(StationState::Connected);
                        startTime = std::chrono::steady_clock::now();
                        break;
                    }
                }

                if (state_.load() != StationState::Connected) {
                    state_.store(StationState::Error);
                    errorMessage_ = "Reconnection failed";
                    return;
                }
                continue;
            }

            kbytesSent_.store(kbytesSent_.load() + encBytes / 1024.0);
        }

        // Update stream time
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - startTime).count();
        streamStartTime_.store(elapsed);
    }
}

// ─────────────────────────────────────────────
// Server Connection (protocol layer)
// ─────────────────────────────────────────────
int StationConnection::connectToServer() {
    // TODO: Implement Icecast/Shoutcast protocol connection
    // This will call the refactored ic_connect() or sc_connect()
    // from BUTT's icecast.cpp / shoutcast.cpp

    fprintf(stdout, "[Station %d] Connecting to %s:%d...\n",
            config_.id, config_.address.c_str(), config_.port);

    // Placeholder — in production, this connects via TCP socket
    // and performs the Icecast SOURCE/PUT or Shoutcast protocol handshake
    return 0; // 0 = success
}

int StationConnection::sendToServer(const uint8_t* data, int len) {
    // TODO: Call ic_send() or sc_send() depending on serverType
    // For now, simulate success
    return len;
}

void StationConnection::disconnectFromServer() {
    // TODO: Call ic_disconnect() or sc_disconnect()
    if (socket_ >= 0) {
        // close(socket_);
        socket_ = -1;
    }
}

// ─────────────────────────────────────────────
// Encoder Factory
// ─────────────────────────────────────────────
std::unique_ptr<IEncoder> StationConnection::createEncoder(CodecType type) {
    switch (type) {
        case CodecType::MP3:    return std::make_unique<LameEncoder>();
        case CodecType::MP2:    return std::make_unique<TwoLameEncoder>();
        case CodecType::AAC:    return std::make_unique<AacEncoder>();
        case CodecType::Opus:   return std::make_unique<OpusEncoder_>();
        case CodecType::Vorbis: return std::make_unique<VorbisEncoder>();
        case CodecType::FLAC:   return std::make_unique<FlacEncoder>();
        default:                return nullptr;
    }
}

// ─────────────────────────────────────────────
// Status Snapshot
// ─────────────────────────────────────────────
StationStatus StationConnection::status() const {
    StationStatus s;
    s.state = state_.load();
    s.listeners = listenerCount_.load();
    s.streamTimeSecs = streamStartTime_.load();
    s.kbytesSent = kbytesSent_.load();
    s.errorMessage = errorMessage_;
    return s;
}

void StationConnection::updateSongName(const std::string& songName) {
    // TODO: Call ic_update_song() or sc_update_song()
    fprintf(stdout, "[Station %d] Song update: %s\n",
            config_.id, songName.c_str());
}

} // namespace ore
