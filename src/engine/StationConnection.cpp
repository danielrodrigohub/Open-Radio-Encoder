// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Station Connection Implementation
//
// This is the per-station streaming thread, the multi-server equivalent
// of BUTT's snd_stream_thread().
// ═══════════════════════════════════════════════════════════════════════
#include "StationConnection.h"
#include "BroadcastDistributor.h"
#include "encoders/lame_encoder.h"
#include "encoders/twolame_encoder.h"
#include "encoders/aac_encoder.h"
#include "encoders/opus_encoder.h"
#include "encoders/vorbis_encoder.h"
#include "encoders/flac_encoder.h"
#include <chrono>
#include <cmath>

namespace ore {

StationConnection::StationConnection() {
    // Standard buffer sizes
    ringBuffer_.resize(16384 * 8); // ~3 seconds at 44k
    rbCapacity_ = ringBuffer_.size();
    encodeBuffer_.resize(16384);
}

StationConnection::~StationConnection() {
    disconnect();
}

void StationConnection::configure(const StationConfig& config) {
    if (shouldRun_.load()) return;
    config_ = config;
}

// ─────────────────────────────────────────────
// Feed Audio (called from mixer thread)
//
// This is a non-blocking write to the station's ring buffer.
// If the ring buffer is full (station thread is stalling),
// we drop the audio frame. This prevents the mixer thread
// from blocking due to one slow station.
// ─────────────────────────────────────────────
void StationConnection::feedAudio(const float* buffer, int frames, int channels, int sampleRate) {
    inputSampleRate_.store(sampleRate, std::memory_order_relaxed);
    
    size_t samplesToWrite = frames * channels;
    size_t wr = rbWritePos_.load(std::memory_order_relaxed);
    size_t rd = rbReadPos_.load(std::memory_order_acquire);

    auto rbAvailWrite = [](size_t w, size_t r, size_t cap) { return (r + cap - w - 1) % cap; };

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

#ifdef HAVE_SAMPLERATE
    fprintf(stdout, "[Station %d] HAVE_SAMPLERATE is DEFINED\n", config_.id);
    int err = 0;
    resampler_ = src_new(SRC_SINC_BEST_QUALITY, config_.encoder.channels, &err);
    if (!resampler_) {
        fprintf(stderr, "[Station %d] Failed to create resampler: %s\n", 
                config_.id, src_strerror(err));
    } else {
        fprintf(stdout, "[Station %d] Resampler (BEST QUALITY) created successfully\n", config_.id);
    }
    resampleBuffer_.resize(16384); // Larger buffer for best quality
#else
    fprintf(stdout, "[Station %d] HAVE_SAMPLERATE is NOT defined!\n", config_.id);
#endif

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
    if (!shouldRun_.load()) {
        streamStartTime_.store(0.0);
        return;
    }

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

#ifdef HAVE_SAMPLERATE
    if (resampler_) {
        src_delete(resampler_);
        resampler_ = nullptr;
    }
#endif

    state_.store(StationState::Disconnected);
    streamStartTime_.store(0.0);
    kbytesSent_.store(0.0);
    listenerCount_.store(0);
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
//      c. Resample if necessary (libsamplerate)
//      d. Encode (MP3/MP2/AAC/Opus/Vorbis/FLAC)
//      e. Send encoded bytes to server
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

    // Give the server a moment to settle before the first metadata push
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Send initial metadata immediately
    {
        std::lock_guard<std::mutex> lock(metadataMutex_);
        pendingSongName_ = "Open Radio Encoder - Made in Chile with Love!";
        hasPendingMetadata_ = true;
    }
    sendPendingMetadata();

    // Frame size for encoding (same as BUTT's framepacket_size)
    const int frameSize = 1024;  // frames per encoding chunk
    const int channels = config_.encoder.channels;
    const int samplesPerChunk = frameSize * channels;

    std::vector<float> audioBuf(samplesPerChunk);

    auto rbAvailRead = [](size_t w, size_t r, size_t cap) { return (w + cap - r) % cap; };

    // ── Phase 2: Encode + Send loop ──
    while (shouldRun_.load()) {
        // Send pending metadata BEFORE waiting for audio data to ensure it reaches the server
        sendPendingMetadata();

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

        // Read and process all available chunks
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

            // ── Stage 1: Resampling ──
            int inRate = inputSampleRate_.load(std::memory_order_relaxed);
            int outRate = config_.encoder.samplerate;
            
            float* dataToEncode = audioBuf.data();
            int framesToEncode = frameSize;

#ifdef HAVE_SAMPLERATE
            if (resampler_ && inRate > 0 && inRate != outRate) {
                static int resampleLogCount = 0;
                if (resampleLogCount++ % 250 == 0) { // Every ~5 seconds
                    fprintf(stdout, "[Station %d] ACTIVE RESAMPLING: %d -> %d (ratio: %.4f)\n", 
                            config_.id, inRate, outRate, static_cast<double>(outRate) / inRate);
                }

                SRC_DATA srcData;
                srcData.data_in = audioBuf.data();
                srcData.input_frames = frameSize;
                srcData.data_out = resampleBuffer_.data();
                srcData.output_frames = static_cast<long>(resampleBuffer_.size() / channels);
                srcData.src_ratio = static_cast<double>(outRate) / inRate;
                srcData.end_of_input = 0;

                int err = src_process(resampler_, &srcData);
                if (err == 0) {
                    dataToEncode = resampleBuffer_.data();
                    framesToEncode = static_cast<int>(srcData.output_frames_gen);
                } else {
                    fprintf(stderr, "[Station %d] Resampling error: %s\n", 
                            config_.id, src_strerror(err));
                }
            } else if (inRate == outRate) {
                // Optional: log once that we are NOT resampling
                static bool loggedDirect = false;
                if (!loggedDirect) {
                    fprintf(stdout, "[Station %d] No resampling needed (rates match at %d)\n", config_.id, inRate);
                    loggedDirect = true;
                }
            }
#endif

            // ── Stage 2: Encode ──
            // Note: Since resampling can produce a variable number of samples, 
            // most of our encoders (LAME, FDK-AAC, Opus) already have internal accumulation.
            int encBytes = encoder_->encode(
                dataToEncode, framesToEncode,
                encodeBuffer_.data(),
                static_cast<int>(encodeBuffer_.size())
            );

            if (encBytes <= 0) continue;

            // ── Stage 3: Send to server ──
            int sent = sendToServer(encodeBuffer_.data(), encBytes);
            if (sent < 0) {
                juce::Logger::writeToLog("Caida de Red - Estación " + juce::String(config_.id) + " (" + config_.name + ")");

                // Connection lost — try to reconnect
                state_.store(StationState::Reconnecting);
                disconnectFromServer();

                // Backoff retry
                int interval = std::max(1, config_.reconnectInterval);
                for (int retry = 0; retry < 10 && shouldRun_.load(); retry++) {
                    std::this_thread::sleep_for(std::chrono::seconds(interval));
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

            if (encBytes > 0) {
                static int sendCount = 0;
                if (sendCount++ % 100 == 0) {
                    fprintf(stdout, "[Station %d] Sent %d bytes to server\n", config_.id, encBytes);
                }
            }

            kbytesSent_.store(kbytesSent_.load() + encBytes / 1024.0);
        }

        // Update stream time
        auto now = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(now - startTime).count();
        streamStartTime_.store(elapsed);
    }
}

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

int StationConnection::connectToServer() {
    if (config_.serverType == ServerType::Icecast) {
        icecastClient_ = std::make_unique<IcecastClient>();
        IcecastConfig icCfg;
        icCfg.addr = config_.address;
        icCfg.port = config_.port;
        icCfg.mount = config_.mountPoint;
        icCfg.user = config_.username;
        icCfg.password = config_.password;
        icCfg.tls = config_.useTLS;
        icCfg.content_type = config_.encoder.contentType();
        icCfg.bitrate = config_.encoder.bitrate;
        icCfg.samplerate = config_.encoder.samplerate;
        icCfg.channels = config_.encoder.channels;

        int rc = icecastClient_->connect(icCfg);
        if (rc != 0) {
            fprintf(stderr, "[Station %d] Icecast connection failed\n", config_.id);
            icecastClient_.reset();
            return -1;
        }
        return 0;
    } else {
        shoutcastClient_ = std::make_unique<ShoutcastClient>();
        ShoutcastConfig scCfg;
        scCfg.addr = config_.address;
        scCfg.port = config_.port;
        scCfg.user = config_.username;
        scCfg.password = config_.password;
        scCfg.mount = config_.mountPoint;
        scCfg.content_type = config_.encoder.contentType();
        scCfg.bitrate = config_.encoder.bitrate;
        scCfg.samplerate = config_.encoder.samplerate;
        scCfg.channels = config_.encoder.channels;

        int rc = shoutcastClient_->connect(scCfg);
        if (rc != 0) {
            fprintf(stderr, "[Station %d] Shoutcast connection failed\n", config_.id);
            shoutcastClient_.reset();
            return -1;
        }
        return 0;
    }
}

int StationConnection::sendToServer(const uint8_t* data, int len) {
    if (icecastClient_) return icecastClient_->send(data, len);
    if (shoutcastClient_) return shoutcastClient_->send(data, len);
    return -1;
}

void StationConnection::disconnectFromServer() {
    if (icecastClient_) {
        icecastClient_->disconnect();
        icecastClient_.reset();
    }
    if (shoutcastClient_) {
        shoutcastClient_->disconnect();
        shoutcastClient_.reset();
    }
}

StationStatus StationConnection::status() const {
    StationStatus s;
    s.state = state_.load();
    s.streamTimeSecs = streamStartTime_.load();
    s.kbytesSent = kbytesSent_.load();
    s.listeners = listenerCount_.load();
    s.errorMessage = errorMessage_;
    return s;
}

void StationConnection::updateSongName(const std::string& songName) {
    std::string processedName = songName;
    if (processedName == "Sia - Unstoppable") {
        processedName = "Open Radio Encoder - Made in Chile with Love!";
    }

    std::lock_guard<std::mutex> lock(metadataMutex_);
    pendingSongName_ = processedName;
    hasPendingMetadata_ = true;
}

void StationConnection::sendPendingMetadata() {
    std::string song;
    {
        std::lock_guard<std::mutex> lock(metadataMutex_);
        if (!hasPendingMetadata_) return;
        song = pendingSongName_;
        hasPendingMetadata_ = false;
    }

    int result = -1;
    if (icecastClient_) {
        result = icecastClient_->updateSong(song);
    } else if (shoutcastClient_) {
        result = shoutcastClient_->updateSong(song);
    }

    if (result == 0) {
        juce::Logger::writeToLog("Cambios en Nowplaying: " + song);
    }

    if (encoder_) {
        encoder_->updateMetadata(song);
    }
}

} // namespace ore
