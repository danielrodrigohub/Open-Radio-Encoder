#pragma once
#include <JuceHeader.h>
#include "engine/AudioPipeline.h"

namespace ore {

class MixerDialog : public juce::Component, public juce::Timer {
public:
    MixerDialog(AudioPipeline* pipeline);
    ~MixerDialog() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    void drawVUMeter(juce::Graphics& g, juce::Rectangle<float> area, float levelDb, float peakDb,
                     juce::String label);

    AudioPipeline* pipeline_;

    juce::Label header_{"", "Input Monitor"};

    juce::Label deviceLabel_{"", "Audio Device"};
    juce::Label deviceName_{"", ""};

    juce::Label masterLabel_{"", "Master Gain"};
    juce::Slider masterSlider_;
    juce::Label masterValue_{"", "1.00"};

    juce::Label leftLabel_{"", "Left"}, rightLabel_{"", "Right"};
    juce::Label leftDb_{"", "-inf dB"}, rightDb_{"", "-inf dB"};
    juce::Label leftPeak_{"", "Peak --"}, rightPeak_{"", "Peak --"};

    float peakL_ = -100.0f, peakR_ = -100.0f;
    float rmsL_ = -100.0f, rmsR_ = -100.0f;

    float peakHoldL_ = -100.0f, peakHoldR_ = -100.0f;
    int peakHoldCounterL_ = 0, peakHoldCounterR_ = 0;
    static constexpr int kPeakHoldFrames = 180; // ~3 seconds at 60fps
    static constexpr float kPeakDecay = 0.5f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerDialog)
};

} // namespace ore
