// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Main Window
// Top-level DocumentWindow containing all UI panels
// Layout matches the BUTTM 1.0.0 - Pro reference image
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include <JuceHeader.h>
#include "ToolbarComponent.h"
#include "StreamingPanel.h"
#include "CurrentSongPanel.h"
#include "RecordingPanel.h"
#include "VUMeterComponent.h"

namespace ore {

class MainContentComponent : public juce::Component {
public:
    MainContentComponent();
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Top toolbar
    ToolbarComponent toolbar_;

    // Left side panels (streaming area)
    StreamingPanel streamingPanel_;
    CurrentSongPanel currentSongPanel_;
    RecordingPanel recordingPanel_;

    // Right side VU meter
    VUMeterComponent vuMeter_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};

class MainWindow : public juce::DocumentWindow {
public:
    explicit MainWindow(const juce::String& name);
    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace ore
