// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Shoutcast Client
// Implementation using libshout
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include <cstdint>
#include <string>

// Forward declare shout_t from libshout
struct shout;
typedef struct shout shout_t;

namespace ore {

struct ShoutcastConfig {
    std::string addr = "localhost";
    std::string user = "";
    std::string password = "changeme";
    std::string mount = "";
    int port = 8000;
    std::string content_type = "audio/mpeg";
    int bitrate = 128;
    int samplerate = 44100;
    int channels = 2;
};

class ShoutcastClient {
public:
    ShoutcastClient();
    ~ShoutcastClient();

    int connect(const ShoutcastConfig& cfg);
    int send(const uint8_t* data, int len);
    int updateSong(const std::string& song);
    int getListenerCount();
    void disconnect();

private:
    shout_t* shout_ = nullptr;
    bool isConnected_ = false;
};

} // namespace ore
