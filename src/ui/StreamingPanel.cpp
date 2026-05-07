// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Streaming Panel Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "StreamingPanel.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

StreamingPanel::StreamingPanel() {
    // ── Header ──
    headerLabel_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    headerLabel_.setColour(juce::Label::textColourId,
                            juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(headerLabel_);

    // ── Connect All / Disconnect All / Add Station ──
    connectAllBtn_.setButtonText("Connect all");
    disconnectAllBtn_.setButtonText("Disconnect all");

    connectAllBtn_.onClick = [this]() { if (onConnectAll) onConnectAll(); };
    disconnectAllBtn_.onClick = [this]() { if (onDisconnectAll) onDisconnectAll(); };
    addStationBtn_.onClick = [this]() { if (onAddStation) onAddStation(); };

    addAndMakeVisible(connectAllBtn_);
    addAndMakeVisible(disconnectAllBtn_);
    addAndMakeVisible(addStationBtn_);

    // ── Viewport for scrollable station list ──
    viewport_.setViewedComponent(&stationContainer_, false);
    viewport_.setScrollBarsShown(true, false);
    addAndMakeVisible(viewport_);
}

void StreamingPanel::addStation(int id, const juce::String& name,
                                 const juce::String& serverType,
                                 const juce::String& encoderType) {
    auto block = std::make_unique<StationBlock>(id, name, serverType, encoderType);
    stationContainer_.addAndMakeVisible(block.get());
    stationBlocks_.push_back(std::move(block));
    resized();
}

void StreamingPanel::clearStations() {
    stationBlocks_.clear();
    stationContainer_.removeAllChildren();
    resized();
}

void StreamingPanel::paint(juce::Graphics& g) {
    // Transparent — parent draws background
}

void StreamingPanel::resized() {
    auto bounds = getLocalBounds();

    // Header
    headerLabel_.setBounds(bounds.removeFromTop(22));

    // Connect All / Disconnect All / Add Station row
    auto buttonRow = bounds.removeFromTop(40).reduced(0, 4);
    addStationBtn_.setBounds(buttonRow.removeFromRight(36).reduced(2, 0));
    buttonRow.removeFromRight(6);
    auto halfW = buttonRow.getWidth() / 2;
    connectAllBtn_.setBounds(buttonRow.removeFromLeft(halfW).reduced(2, 0));
    disconnectAllBtn_.setBounds(buttonRow.reduced(2, 0));

    bounds.removeFromTop(5);

    // Station blocks in scrollable viewport
    viewport_.setBounds(bounds);

    // Layout station blocks vertically inside the container
    int blockHeight = 110;
    int totalHeight = static_cast<int>(stationBlocks_.size()) * (blockHeight + 8);
    stationContainer_.setBounds(0, 0, viewport_.getWidth() - 10, totalHeight);

    int y = 0;
    for (auto& block : stationBlocks_) {
        block->setBounds(0, y, stationContainer_.getWidth(), blockHeight);
        y += blockHeight + 8;
    }
}

} // namespace ore
