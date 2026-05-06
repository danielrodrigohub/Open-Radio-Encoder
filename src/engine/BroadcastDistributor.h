// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Broadcast Distributor
// Multi-Server Engine: Distributes processed audio to N concurrent
// encoder/streaming threads.
//
// This is the NEW architecture that replaces BUTT's single-server design.
// BUTT's snd_stream_thread() handled one connection. We now manage N.
//
// Thread Model:
//   Mixer Thread → distribute() → memcpy to N ring buffers
//   N StationConnection threads → each reads its own ring buffer
//                                → encodes independently
//                                → sends via Icecast/Shoutcast
// ═══════════════════════════════════════════════════════════════════════
#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "encoders/encoder_interface.h"

namespace ore {

class StationConnection;

/// Server protocol type
enum class ServerType {
    Icecast,
    Shoutcast
};

/// Configuration for a single station/server connection
struct StationConfig {
    int         id = 0;
    bool        enabled = true;
    std::string name = "Station #1";

    // Server
    ServerType  serverType = ServerType::Icecast;
    std::string address = "localhost";
    int         port = 8000;
    std::string mountPoint = "/stream";
    std::string username = "source";
    std::string password = "";
    bool        useTLS = false;

    // Encoder
    EncoderConfig encoder;

    // ICY metadata
    std::string icyName = "";
    std::string icyGenre = "";
    std::string icyUrl = "";
    bool        icyPublic = false;
};

/// Connection state for a single station
enum class StationState {
    Disconnected,
    Connecting,
    Connected,      // "On Air"
    Reconnecting,
    Error
};

/// Real-time status snapshot of a station (for UI display)
struct StationStatus {
    StationState state = StationState::Disconnected;
    int          listeners = 0;
    double       streamTimeSecs = 0.0;
    double       kbytesSent = 0.0;
    std::string  errorMessage;
};

/// The Broadcast Distributor manages N station connections.
///
/// Usage:
///   1. addStation(config)  — add station configs
///   2. connectAll()        — launch all threads
///   3. distribute(buffer)  — called every frame from mixer thread
///   4. disconnectAll()     — stop all threads
class BroadcastDistributor {
public:
    BroadcastDistributor();
    ~BroadcastDistributor();

    // ── Station Management ──

    /// Add a new station. Returns the station ID.
    int addStation(const StationConfig& config);

    /// Remove a station by ID. Disconnects first if connected.
    void removeStation(int stationId);

    /// Update a station's configuration. Must disconnect first.
    void updateStation(int stationId, const StationConfig& config);

    /// Get number of stations.
    int stationCount() const;

    /// Get config for a station.
    const StationConfig& stationConfig(int index) const;

    // ── Connection Control ──

    /// Connect a single station.
    void connectStation(int stationId);

    /// Disconnect a single station.
    void disconnectStation(int stationId);

    /// Connect all enabled stations ("Connect All" button).
    void connectAll();

    /// Disconnect all stations ("Disconnect All" button).
    void disconnectAll();

    // ── Audio Distribution ──

    /// Distribute a processed audio frame to all connected stations.
    /// Called from the mixer thread. Copies buffer to each station's ring buffer.
    /// @param buffer  Interleaved float32 PCM
    /// @param frames  Number of frames (samples per channel)
    /// @param channels Number of channels
    void distribute(const float* buffer, int frames, int channels);

    // ── Status ──

    /// Get the current status of a station (thread-safe snapshot).
    StationStatus getStationStatus(int stationId) const;

    // ── Metadata ──

    /// Update the current song name on all connected stations.
    void updateSongName(const std::string& songName);

private:
    mutable std::mutex mutex_;
    std::vector<std::unique_ptr<StationConnection>> stations_;
    int nextId_ = 1;
};

} // namespace ore
