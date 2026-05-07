#include "CurrentSongPanel.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

CurrentSongPanel::CurrentSongPanel() {
    headerLabel_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    headerLabel_.setColour(juce::Label::textColourId,
                            juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(headerLabel_);

    songEditor_.setText("Open Radio Encoder - Made in Chile with Love!");
    songEditor_.setMultiLine(false);
    songEditor_.setFont(juce::Font(juce::FontOptions(13.0f)));
    addAndMakeVisible(songEditor_);

    updateBtn_.onClick = [this]() {
        if (onUpdateSong)
            onUpdateSong(songEditor_.getText());
    };
    addAndMakeVisible(updateBtn_);

    // TXT file mode
    txtModeToggle_.onClick = [this]() {
        bool enabled = txtModeToggle_.getToggleState();
        txtPathEditor_.setEnabled(enabled);
        txtBrowseBtn_.setEnabled(enabled);
        songEditor_.setEnabled(!enabled);
        updateBtn_.setEnabled(!enabled);
    };
    addAndMakeVisible(txtModeToggle_);

    txtPathEditor_.setText("nowplaying.txt");
    txtPathEditor_.setFont(juce::Font(juce::FontOptions(12.0f)));
    txtPathEditor_.setEnabled(false);
    addAndMakeVisible(txtPathEditor_);

    txtBrowseBtn_.onClick = [this]() { browseForFile(); };
    txtBrowseBtn_.setEnabled(false);
    addAndMakeVisible(txtBrowseBtn_);

    txtStatusLabel_.setFont(juce::Font(juce::FontOptions(9.0f)));
    txtStatusLabel_.setColour(juce::Label::textColourId,
                               juce::Colour(LookAndFeel_OpenRadio::kTextDim));
    addAndMakeVisible(txtStatusLabel_);
}

void CurrentSongPanel::paint(juce::Graphics& g) {
    // Transparent
}

void CurrentSongPanel::resized() {
    auto bounds = getLocalBounds();

    headerLabel_.setBounds(bounds.removeFromTop(22));
    bounds.removeFromTop(4);

    auto row = bounds.removeFromTop(30);
    updateBtn_.setBounds(row.removeFromRight(100).reduced(2, 0));
    row.removeFromRight(6);
    songEditor_.setBounds(row);

    bounds.removeFromTop(4);

    // TXT file row
    auto txtRow = bounds.removeFromTop(28);
    txtModeToggle_.setBounds(txtRow.removeFromLeft(70));
    txtRow.removeFromLeft(4);
    txtBrowseBtn_.setBounds(txtRow.removeFromRight(36));
    txtRow.removeFromRight(2);
    txtPathEditor_.setBounds(txtRow);

    txtStatusLabel_.setBounds(bounds.removeFromTop(14));
}

void CurrentSongPanel::browseForFile() {
    juce::FileChooser chooser("Select Now Playing file",
        juce::File(txtPathEditor_.getText()),
        "*.txt");
    if (chooser.browseForFileToOpen()) {
        txtPathEditor_.setText(chooser.getResult().getFullPathName());
    }
}

void CurrentSongPanel::pollFileIfNeeded() {
    if (!txtModeToggle_.getToggleState()) return;

    juce::File file(txtPathEditor_.getText());
    if (!file.existsAsFile()) {
        txtStatusLabel_.setText("File not found: " + file.getFullPathName(),
                                 juce::dontSendNotification);
        return;
    }

    auto modTime = file.getLastModificationTime();
    if (modTime == lastModTime_) return;

    lastModTime_ = modTime;
    juce::String content = file.loadFileAsString().trim();

    if (content.isEmpty()) {
        txtStatusLabel_.setText("File is empty", juce::dontSendNotification);
        return;
    }

    if (content != lastTxtContent_) {
        lastTxtContent_ = content;
        songEditor_.setText(content, juce::dontSendNotification);

        if (onUpdateSong) {
            onUpdateSong(content);
        }

        auto now = juce::Time::getCurrentTime();
        txtStatusLabel_.setText("Updated: " + now.formatted("%H:%M:%S") + " | " + content.substring(0, 40),
                                 juce::dontSendNotification);
    }
}

} // namespace ore
