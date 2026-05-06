// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Current Song Panel Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "CurrentSongPanel.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

CurrentSongPanel::CurrentSongPanel() {
    headerLabel_.setFont(juce::Font(14.0f, juce::Font::bold));
    headerLabel_.setColour(juce::Label::textColourId,
                            juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(headerLabel_);

    songEditor_.setText("Sia - Unstoppable");
    songEditor_.setMultiLine(false);
    songEditor_.setFont(juce::Font(13.0f));
    addAndMakeVisible(songEditor_);

    updateBtn_.onClick = [this]() {
        if (onUpdateSong)
            onUpdateSong(songEditor_.getText());
    };
    addAndMakeVisible(updateBtn_);
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
}

} // namespace ore
