#include "opus_encoder.h"
namespace ore {
int OpusEncoder_::init(const EncoderConfig& config) { config_ = config; return 0; }
int OpusEncoder_::encode(const float*, int, uint8_t*, int) { return 0; }
void OpusEncoder_::close() {}
} // namespace ore
