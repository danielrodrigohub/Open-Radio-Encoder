#include "uri_encode.h"
#include <cstring>
namespace ore {
void uri_encode(const char* src, size_t len, char* dst) { memcpy(dst, src, len); dst[len] = '\0'; }
} // namespace ore
