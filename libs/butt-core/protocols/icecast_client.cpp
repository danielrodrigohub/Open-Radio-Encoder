#include "icecast_client.h"
namespace ore {
int IcecastClient::connect(const IcecastConfig&) { return 0; }
int IcecastClient::send(const uint8_t*, int len) { return len; }
int IcecastClient::updateSong(const std::string&) { return 0; }
int IcecastClient::getListenerCount() { return -1; }
void IcecastClient::disconnect() {}
} // namespace ore
