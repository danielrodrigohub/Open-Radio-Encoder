#pragma once
// cJSON stub — lightweight JSON parser (from BUTT)
namespace ore {
struct cJSON { int type; double valuedouble; int valueint; char* valuestring; cJSON* child; cJSON* next; };
cJSON* cJSON_Parse(const char* value);
void cJSON_Delete(cJSON* item);
cJSON* cJSON_GetObjectItemCaseSensitive(const cJSON*, const char*);
int cJSON_IsNumber(const cJSON*);
int cJSON_IsString(const cJSON*);
int cJSON_IsArray(const cJSON*);
char* cJSON_GetStringValue(const cJSON*);
} // namespace ore
