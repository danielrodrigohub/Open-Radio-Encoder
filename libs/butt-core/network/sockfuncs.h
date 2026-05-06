// Socket functions — stub (port from BUTT sockfuncs.h)
#pragma once
namespace ore {
int sock_init();
int sock_connect(const char* addr, int port, int proto, int timeout);
int sock_send(int socket, const char* buf, int len, int timeout);
int sock_recv(int socket, char* buf, int len, int timeout);
void sock_close(int socket);
} // namespace ore
