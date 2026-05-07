// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Icecast Client
// Implementation using libshout
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include <cstdint>
#include <string>

// Forward declare shout_t from libshout
struct shout;
typedef struct shout shout_t;

namespace ore {

struct IcecastConfig {
    std::string addr = "localhost";
    std::string mount = "/stream";
    std::string user = "source";
    std::string password = "hackme";
    int port = 8000;
    bool tls = false;
    std::string content_type = "audio/mpeg"; // audio/mpeg, audio/aac, audio/ogg
    int bitrate = 128;
    int samplerate = 44100;
    int channels = 2;
};

class IcecastClient {
public:
    IcecastClient();
    ~IcecastClient();

    /// Connects to the Icecast server. Returns 0 on success, < 0 on error.
    int connect(const IcecastConfig& cfg);

    /// Sends encoded data to the server. Returns bytes sent, or < 0 on error.
    int send(const uint8_t* data, int len);

    /// Updates the metadata (current song). Returns 0 on success.
    int updateSong(const std::string& song);

    /// Fetches the listener count. (libshout doesn't provide this directly, usually requires an HTTP API call)
    /// Returns -1 if unsupported.
    int getListenerCount();

    /// Disconnects from the server.
    void disconnect();

private:
    shout_t* shout_ = nullptr;
    bool isConnected_ = false;
};

} // namespace ore
