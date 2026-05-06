// AAC Encoder - stub implementation (port from BUTT aac_encode.cpp)
#include "aac_encoder.h"
namespace ore {
int AacEncoder::init(const EncoderConfig& config) { config_ = config; return 0; }
int AacEncoder::encode(const float*, int, uint8_t*, int) { return 0; }
void AacEncoder::close() {}
} // namespace ore
