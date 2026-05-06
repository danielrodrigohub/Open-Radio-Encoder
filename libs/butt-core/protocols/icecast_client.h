// Icecast client stub — port from BUTT icecast.cpp
#pragma once
#include <string>
namespace ore {
struct IcecastConfig {
    std::string addr, mount, user = "source", password;
    int port = 8000;
    bool tls = false;
    std::string content_type = "audio/mpeg";
};
class IcecastClient {
public:
    int connect(const IcecastConfig& cfg);
    int send(const uint8_t* data, int len);
    int updateSong(const std::string& song);
    int getListenerCount();
    void disconnect();
private:
    int socket_ = -1;
};
} // namespace ore
