// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Streaming Panel
// Contains: "Connect All" / "Disconnect All" buttons + N StationBlocks
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include <JuceHeader.h>
#include "StationBlock.h"

namespace ore {

class StreamingPanel : public juce::Component {
public:
    StreamingPanel();
    void paint(juce::Graphics& g) override;
    void resized() override;

    /// Add a station block dynamically.
    void addStation(int id, const juce::String& name,
                    const juce::String& serverType,
                    const juce::String& encoderType);

    /// Remove all station blocks.
    void clearStations();

    /// Get station blocks for wiring callbacks.
    const std::vector<std::unique_ptr<StationBlock>>& stationBlocks() const {
        return stationBlocks_;
    }
    std::vector<std::unique_ptr<StationBlock>>& stationBlocks() {
        return stationBlocks_;
    }

    /// Callbacks
    std::function<void()> onConnectAll;
    std::function<void()> onDisconnectAll;
    std::function<void()> onAddStation;

private:
    juce::Label headerLabel_{"", "Streaming"};
    juce::TextButton connectAllBtn_;
    juce::TextButton disconnectAllBtn_;
    juce::TextButton addStationBtn_{"+"};

    std::vector<std::unique_ptr<StationBlock>> stationBlocks_;

    // Scrollable viewport for station blocks
    juce::Viewport viewport_;
    juce::Component stationContainer_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StreamingPanel)
};

} // namespace ore
