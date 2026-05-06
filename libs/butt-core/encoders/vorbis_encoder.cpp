#include "vorbis_encoder.h"
namespace ore {
int VorbisEncoder::init(const EncoderConfig& config) { config_ = config; return 0; }
int VorbisEncoder::encode(const float*, int, uint8_t*, int) { return 0; }
void VorbisEncoder::close() {}
} // namespace ore
