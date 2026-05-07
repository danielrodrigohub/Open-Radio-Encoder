#pragma once
#include <JuceHeader.h>
#include "engine/AudioPipeline.h"

namespace ore {

class AudioFxDialog : public juce::Component {
public:
    AudioFxDialog(AudioPipeline* pipeline);
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    AudioPipeline* pipeline_;
    DSPEffects* dsp_;

    static constexpr int kBandCount = 10;
    static const char* kBandNames[kBandCount];

    juce::ToggleButton eqBypass_{"EQ Enable"};
    juce::Slider eqSliders_[kBandCount];
    juce::Label eqLabels_[kBandCount];
    juce::Label eqHeader_{"", "10-Band Equalizer"};

    juce::ToggleButton compBypass_{"Compressor Enable"};
    juce::Slider thresholdSlider_, ratioSlider_, attackSlider_, releaseSlider_, makeupSlider_;
    juce::Label thresholdLabel_, ratioLabel_, attackLabel_, releaseLabel_, makeupLabel_;
    juce::Label compHeader_{"", "Compressor"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioFxDialog)
};

} // namespace ore
