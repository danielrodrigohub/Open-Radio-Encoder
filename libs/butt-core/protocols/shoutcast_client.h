#pragma once
#include <string>
namespace ore {
struct ShoutcastConfig {
    std::string addr, password;
    int port = 8000;
    std::string content_type = "audio/mpeg";
};
class ShoutcastClient {
public:
    int connect(const ShoutcastConfig& cfg);
    int send(const uint8_t* data, int len);
    int updateSong(const std::string& song);
    int getListenerCount();
    void disconnect();
private:
    int socket_ = -1;
};
} // namespace ore
