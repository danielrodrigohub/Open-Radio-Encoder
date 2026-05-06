// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Station Block Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "StationBlock.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

StationBlock::StationBlock(int stationId, const juce::String& name,
                           const juce::String& serverType,
                           const juce::String& encoderType)
    : stationId_(stationId)
{
    // ── Checkbox + Title ──
    enabledCheck_.setButtonText("");
    enabledCheck_.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(enabledCheck_);

    auto title = juce::String("Station #") + juce::String(stationId)
                 + " [ " + serverType + " | " + encoderType + " Encoder ]";
    titleLabel_.setText(title, juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(13.0f, juce::Font::bold));
    titleLabel_.setColour(juce::Label::textColourId,
                          juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(titleLabel_);

    // ── Buttons ──
    addAndMakeVisible(connectBtn_);
    addAndMakeVisible(disconnectBtn_);

    connectBtn_.onClick = [this]() { if (onConnect) onConnect(); };
    disconnectBtn_.onClick = [this]() { if (onDisconnect) onDisconnect(); };

    // ── Stream Time ──
    streamTimeLabelHeader_.setFont(juce::Font(11.0f));
    streamTimeLabelHeader_.setColour(juce::Label::textColourId,
                                     juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(streamTimeLabelHeader_);

    streamTimeLabel_.setText("--:--:--", juce::dontSendNotification);
    streamTimeLabel_.setFont(juce::Font("Courier New", 22.0f, juce::Font::bold));
    streamTimeLabel_.setColour(juce::Label::textColourId,
                                juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    addAndMakeVisible(streamTimeLabel_);

    // ── Listeners ──
    listenersLabelHeader_.setFont(juce::Font(11.0f));
    listenersLabelHeader_.setColour(juce::Label::textColourId,
                                     juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(listenersLabelHeader_);

    listenersLabel_.setText("0", juce::dontSendNotification);
    listenersLabel_.setFont(juce::Font(22.0f, juce::Font::bold));
    listenersLabel_.setColour(juce::Label::textColourId,
                               juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(listenersLabel_);
}

void StationBlock::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();

    // Panel background
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kPanel));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Border
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void StationBlock::resized() {
    auto bounds = getLocalBounds().reduced(10, 8);

    // Row 1: Checkbox + Title
    auto row1 = bounds.removeFromTop(22);
    enabledCheck_.setBounds(row1.removeFromLeft(22));
    titleLabel_.setBounds(row1);

    bounds.removeFromTop(6);

    // Row 2+3: Buttons on left, status on right
    auto leftCol = bounds.removeFromLeft(bounds.getWidth() / 2);
    auto rightCol = bounds;

    // Connect / Disconnect buttons
    connectBtn_.setBounds(leftCol.removeFromTop(30).reduced(0, 2));
    leftCol.removeFromTop(4);
    disconnectBtn_.setBounds(leftCol.removeFromTop(30).reduced(0, 2));

    // Stream Time + Listeners
    auto timeCol = rightCol.removeFromLeft(rightCol.getWidth() * 2 / 3);
    auto listCol = rightCol;

    streamTimeLabelHeader_.setBounds(timeCol.removeFromTop(16));
    streamTimeLabel_.setBounds(timeCol);

    listenersLabelHeader_.setBounds(listCol.removeFromTop(16));
    listenersLabel_.setBounds(listCol);
}

void StationBlock::setStreamTime(const juce::String& time) {
    streamTimeLabel_.setText(time, juce::dontSendNotification);
}

void StationBlock::setStreamStatus(const juce::String& status) {
    streamTimeLabel_.setText(status, juce::dontSendNotification);
    if (status == "On Air") {
        streamTimeLabel_.setColour(juce::Label::textColourId,
                                    juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    }
}

void StationBlock::setListenerCount(int count) {
    listenersLabel_.setText(juce::String(count), juce::dontSendNotification);
}

void StationBlock::setConnected(bool connected) {
    isConnected_ = connected;
    connectBtn_.setEnabled(!connected);
    disconnectBtn_.setEnabled(connected);
}

} // namespace ore
