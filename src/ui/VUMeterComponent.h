// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — VU Meter Component
// High-performance vertical VU meter with 60fps refresh
// Range: -54 dB to 0 dB
// Zones: Green (-54 to -12), Yellow (-12 to -6), Red (-6 to 0)
// Features: Smooth ballistics, peak hold indicators
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include <JuceHeader.h>

namespace ore {

class VUMeterComponent : public juce::Component, private juce::Timer {
public:
    VUMeterComponent();
    ~VUMeterComponent() override = default;

    /// Set the current dB levels (called from audio thread via atomic).
    void setLevels(float peakDbL, float peakDbR, float rmsDbL, float rmsDbR);

    void paint(juce::Graphics& g) override;
    void resized() override {}

private:
    void timerCallback() override;

    // Convert dB to normalized position [0, 1]
    float dbToPosition(float dB) const;

    // Get the gradient color for a given position
    juce::Colour getColorForPosition(float pos) const;

    // Current display values (smoothed)
    float displayL_ = -100.0f;
    float displayR_ = -100.0f;

    // Peak hold
    float peakHoldL_ = -100.0f;
    float peakHoldR_ = -100.0f;
    int peakHoldCounterL_ = 0;
    int peakHoldCounterR_ = 0;
    static constexpr int kPeakHoldFrames = 30; // ~0.5 sec at 60fps

    // Target values (set by setLevels)
    std::atomic<float> targetL_{-100.0f};
    std::atomic<float> targetR_{-100.0f};

    // Ballistics
    static constexpr float kAttackCoeff  = 0.7f;   // Fast attack
    static constexpr float kReleaseCoeff = 0.92f;   // Slower release
    static constexpr float kMinDb        = -54.0f;
    static constexpr float kMaxDb        = 0.0f;

    // Scale label positions
    struct ScaleMark {
        float dB;
        juce::String label;
    };
    std::vector<ScaleMark> scaleMarks_;
};

} // namespace ore
