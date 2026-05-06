// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Recording Panel Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "RecordingPanel.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

RecordingPanel::RecordingPanel() {
    headerLabel_.setFont(juce::Font(14.0f, juce::Font::bold));
    headerLabel_.setColour(juce::Label::textColourId,
                            juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(headerLabel_);

    addAndMakeVisible(startBtn_);
    addAndMakeVisible(stopBtn_);

    browseBtn_.setButtonText(juce::String::charToString(0x1F4C2)); // 📂
    addAndMakeVisible(browseBtn_);

    startBtn_.onClick = [this]() { if (onStartRecording) onStartRecording(); };
    stopBtn_.onClick = [this]() { if (onStopRecording) onStopRecording(); };

    // ── Recording Data size ──
    dataSizeLabelHeader_.setFont(juce::Font(11.0f));
    dataSizeLabelHeader_.setColour(juce::Label::textColourId,
                                    juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(dataSizeLabelHeader_);

    dataSizeLabel_.setText("6.71 MB", juce::dontSendNotification);
    dataSizeLabel_.setFont(juce::Font("Courier New", 22.0f, juce::Font::bold));
    dataSizeLabel_.setColour(juce::Label::textColourId,
                              juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    addAndMakeVisible(dataSizeLabel_);

    // ── File path ──
    pathLabel_.setText("Recording to: ~/Music/recordings/rec_20251019_14_28_12.mp3",
                       juce::dontSendNotification);
    pathLabel_.setFont(juce::Font(10.0f));
    pathLabel_.setColour(juce::Label::textColourId,
                          juce::Colour(LookAndFeel_OpenRadio::kTextDim));
    addAndMakeVisible(pathLabel_);
}

void RecordingPanel::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kPanel));
    g.fillRoundedRectangle(bounds, 4.0f);
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void RecordingPanel::resized() {
    auto bounds = getLocalBounds().reduced(10, 8);

    headerLabel_.setBounds(bounds.removeFromTop(22));
    bounds.removeFromTop(6);

    // Buttons row
    auto btnRow = bounds.removeFromTop(30);
    startBtn_.setBounds(btnRow.removeFromLeft(140).reduced(0, 2));
    btnRow.removeFromLeft(6);
    stopBtn_.setBounds(btnRow.removeFromLeft(140).reduced(0, 2));
    btnRow.removeFromLeft(6);
    browseBtn_.setBounds(btnRow.removeFromLeft(36).reduced(0, 2));

    // Data size (right side of button row)
    auto dataArea = bounds.removeFromTop(50);
    auto dataLeft = dataArea.removeFromLeft(dataArea.getWidth() / 2);
    // empty left side
    dataSizeLabelHeader_.setBounds(dataArea.removeFromTop(16));
    dataSizeLabel_.setBounds(dataArea);

    bounds.removeFromTop(4);

    // File path
    pathLabel_.setBounds(bounds.removeFromTop(16));
}

void RecordingPanel::setRecordingSize(const juce::String& size) {
    dataSizeLabel_.setText(size, juce::dontSendNotification);
}

void RecordingPanel::setRecordingPath(const juce::String& path) {
    pathLabel_.setText("Recording to: " + path, juce::dontSendNotification);
}

} // namespace ore
