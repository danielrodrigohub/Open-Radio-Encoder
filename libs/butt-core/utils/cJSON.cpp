#include "cJSON.h"
namespace ore {
cJSON* cJSON_Parse(const char*) { return nullptr; }
void cJSON_Delete(cJSON*) {}
cJSON* cJSON_GetObjectItemCaseSensitive(const cJSON*, const char*) { return nullptr; }
int cJSON_IsNumber(const cJSON*) { return 0; }
int cJSON_IsString(const cJSON*) { return 0; }
int cJSON_IsArray(const cJSON*) { return 0; }
char* cJSON_GetStringValue(const cJSON*) { return nullptr; }
} // namespace ore
