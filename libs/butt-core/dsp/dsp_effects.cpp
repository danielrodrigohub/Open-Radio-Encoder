// DSP Effects — stub implementation
#include "dsp_effects.h"
#include "biquad.h"
namespace ore {
DSPEffects::DSPEffects(uint32_t, uint8_t ch, uint32_t sr) : samplerate_(sr), channels_(ch) {}
DSPEffects::~DSPEffects() {}
bool DSPEffects::hasToProcessSamples() const { return eq_active_ || drc_active_ || gain_ != 1.0f; }
void DSPEffects::processSamples(float*, uint32_t) {}
void DSPEffects::setEQband(int, double) {}
void DSPEffects::resetCompressor() { prev_power_ = 1.0f; prev_gain_dB_ = 0.0f; }
void DSPEffects::compress(float*, uint32_t) {}
} // namespace ore
