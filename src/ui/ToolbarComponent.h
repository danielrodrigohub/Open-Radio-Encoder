// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Toolbar Component
// Icon toolbar: Settings | Scheduler | Station Manager | Server Manager
//               Encoder Manager | Audio Effects | Audio Mixer
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include <JuceHeader.h>

namespace ore {

class ToolbarComponent : public juce::Component {
public:
    ToolbarComponent();
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    struct ToolbarButton {
        std::unique_ptr<juce::TextButton> button;
        juce::String label;
        juce::String icon; // Unicode icon
    };

    std::vector<ToolbarButton> buttons_;

    void addToolbarButton(const juce::String& label, const juce::String& icon);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarComponent)
};

} // namespace ore
