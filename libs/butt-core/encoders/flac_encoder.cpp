#include "flac_encoder.h"
namespace ore {
int FlacEncoder::init(const EncoderConfig& config) { config_ = config; return 0; }
int FlacEncoder::encode(const float*, int, uint8_t*, int) { return 0; }
void FlacEncoder::close() {}
} // namespace ore
