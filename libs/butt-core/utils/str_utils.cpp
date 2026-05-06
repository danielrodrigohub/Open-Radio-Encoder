#include "str_utils.h"
#include <cctype>
namespace ore {
char* strtolower(char* s) { for (char* p = s; *p; p++) *p = tolower(*p); return s; }
char* strtoupper(char* s) { for (char* p = s; *p; p++) *p = toupper(*p); return s; }
} // namespace ore
