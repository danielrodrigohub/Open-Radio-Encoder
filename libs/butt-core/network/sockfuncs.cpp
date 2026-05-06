#include "sockfuncs.h"
namespace ore {
int sock_init() { return 0; }
int sock_connect(const char*, int, int, int) { return -1; }
int sock_send(int, const char*, int, int) { return 0; }
int sock_recv(int, char*, int, int) { return 0; }
void sock_close(int) {}
} // namespace ore
