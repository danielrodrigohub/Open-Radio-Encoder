#include "shoutcast_client.h"
namespace ore {
int ShoutcastClient::connect(const ShoutcastConfig&) { return 0; }
int ShoutcastClient::send(const uint8_t*, int len) { return len; }
int ShoutcastClient::updateSong(const std::string&) { return 0; }
int ShoutcastClient::getListenerCount() { return -1; }
void ShoutcastClient::disconnect() {}
} // namespace ore
