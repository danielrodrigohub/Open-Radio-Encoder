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

    auto title = name + " [ " + serverType + " | " + encoderType + " ]";
    titleLabel_.setText(title, juce::dontSendNotification);
    titleLabel_.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    titleLabel_.setColour(juce::Label::textColourId,
                          juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(titleLabel_);

    // ── Buttons ──
    addAndMakeVisible(connectBtn_);
    addAndMakeVisible(disconnectBtn_);

    connectBtn_.onClick = [this]() { if (onConnect) onConnect(); };
    disconnectBtn_.onClick = [this]() { if (onDisconnect) onDisconnect(); };

    editBtn_.setButtonText("...");
    editBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00444444));
    editBtn_.setTooltip("Edit station configuration");
    editBtn_.onClick = [this]() { if (onEdit) onEdit(); };
    addAndMakeVisible(editBtn_);

    deleteBtn_.setButtonText("X");
    deleteBtn_.setColour(juce::TextButton::buttonColourId,
                          juce::Colour(0xCC333333));
    deleteBtn_.onClick = [this]() { if (onDelete) onDelete(); };
    addAndMakeVisible(deleteBtn_);

    // ── Stream Time ──
    streamTimeLabelHeader_.setFont(juce::Font(juce::FontOptions(11.0f)));
    streamTimeLabelHeader_.setColour(juce::Label::textColourId,
                                     juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(streamTimeLabelHeader_);

    streamTimeLabel_.setText("--:--:--", juce::dontSendNotification);
    streamTimeLabel_.setFont(juce::Font(juce::FontOptions("Courier New", 22.0f, juce::Font::bold)));
    streamTimeLabel_.setColour(juce::Label::textColourId,
                                juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    addAndMakeVisible(streamTimeLabel_);

    // ── Listeners ──
    listenersLabelHeader_.setFont(juce::Font(juce::FontOptions(11.0f)));
    listenersLabelHeader_.setColour(juce::Label::textColourId,
                                     juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(listenersLabelHeader_);

    listenersLabel_.setText("0", juce::dontSendNotification);
    listenersLabel_.setFont(juce::Font(juce::FontOptions(22.0f, juce::Font::bold)));
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

    // Row 1: Checkbox + Title + Edit + Delete button
    auto row1 = bounds.removeFromTop(22);
    enabledCheck_.setBounds(row1.removeFromLeft(22));
    deleteBtn_.setBounds(row1.removeFromRight(28).reduced(0, 2));
    row1.removeFromRight(4);
    editBtn_.setBounds(row1.removeFromRight(28).reduced(0, 2));
    row1.removeFromRight(4);
    titleLabel_.setBounds(row1);

    bounds.removeFromTop(6);

    // Row 2: Status area
    auto statusRow = bounds.removeFromTop(40);
    auto timeCol = statusRow.removeFromLeft(statusRow.getWidth() * 0.7f);
    auto listCol = statusRow;

    streamTimeLabelHeader_.setBounds(timeCol.removeFromTop(14));
    streamTimeLabel_.setBounds(timeCol);

    listenersLabelHeader_.setBounds(listCol.removeFromTop(14));
    listenersLabel_.setBounds(listCol);

    bounds.removeFromTop(6);

    // Row 3: Action buttons (side by side)
    auto btnRow = bounds.removeFromTop(28);
    int btnW = (btnRow.getWidth() - 8) / 2;
    connectBtn_.setBounds(btnRow.removeFromLeft(btnW));
    btnRow.removeFromLeft(8);
    disconnectBtn_.setBounds(btnRow);
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
    
    if (!connected) {
        streamTimeLabel_.setText("--:--:--", juce::dontSendNotification);
    }
}

} // namespace ore
