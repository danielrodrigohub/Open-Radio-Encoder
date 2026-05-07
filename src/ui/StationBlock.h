// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Station Block (Individual Station UI)
// Each station has: checkbox, name label, protocol/encoder label,
// Connect button, Disconnect button, Stream Time, Listener Count
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include <JuceHeader.h>

namespace ore {

class StationBlock : public juce::Component {
public:
    StationBlock(int stationId, const juce::String& name,
                 const juce::String& serverType, const juce::String& encoderType);

    void paint(juce::Graphics& g) override;
    void resized() override;

    int stationId() const { return stationId_; }

    /// Update the display with real-time status data.
    void setStreamTime(const juce::String& time);
    void setStreamStatus(const juce::String& status);
    void setListenerCount(int count);
    void setConnected(bool connected);

    /// Callbacks
    std::function<void()> onConnect;
    std::function<void()> onDisconnect;
    std::function<void()> onDelete;
    std::function<void()> onEdit;

private:
    int stationId_;

    juce::ToggleButton enabledCheck_;
    juce::Label titleLabel_;
    juce::TextButton connectBtn_{"Connect"};
    juce::TextButton disconnectBtn_{"Disconnect"};
    juce::TextButton deleteBtn_{"X"};
    juce::TextButton editBtn_{""};
    juce::Label streamTimeLabel_;
    juce::Label streamTimeLabelHeader_{"", "Stream Time"};
    juce::Label listenersLabel_;
    juce::Label listenersLabelHeader_{"", "Listeners"};

    bool isConnected_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StationBlock)
};

} // namespace ore
