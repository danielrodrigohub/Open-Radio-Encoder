#include "RecordingPanel.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

static bool isLossless(const juce::String& codec) {
    return codec == "wav" || codec == "flac" || codec == "aiff";
}

RecordingPanel::RecordingPanel() {
    headerLabel_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    headerLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(headerLabel_);

    // Codec
    codecLabel_.setFont(juce::Font(juce::FontOptions(10.0f)));
    codecLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(codecLabel_);

    codecBox_.addItem("WAV", 1);  codecBox_.addItem("FLAC", 2);  codecBox_.addItem("AIFF", 3);
    codecBox_.addItem("MP3", 4);  codecBox_.addItem("Opus", 5);  codecBox_.addItem("MP2", 6);
    codecBox_.setSelectedId(1);
    codecBox_.onChange = [this]() {
        int id = codecBox_.getSelectedId();
        switch (id) {
            case 2: currentCodec_ = "flac"; break;
            case 3: currentCodec_ = "aiff"; break;
            case 4: currentCodec_ = "mp3";  break;
            case 5: currentCodec_ = "opus"; break;
            case 6: currentCodec_ = "mp2";  break;
            default: currentCodec_ = "wav"; break;
        }
        updateQualityOptions();
        auto f = juce::File(currentPath_);
        currentPath_ = f.getParentDirectory()
            .getChildFile(f.getFileNameWithoutExtension() + "." + currentCodec_).getFullPathName();
        pathEditor_.setText(currentPath_);
    };
    addAndMakeVisible(codecBox_);

    // Quality
    qualityLabel_.setFont(juce::Font(juce::FontOptions(10.0f)));
    qualityLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(qualityLabel_);
    updateQualityOptions();
    addAndMakeVisible(qualityBox_);

    // Path
    pathEditor_.setFont(juce::Font(juce::FontOptions(12.0f)));
    juce::Time now = juce::Time::getCurrentTime();
    juce::String ts = now.formatted("%Y%m%d_%H%M%S");
    currentPath_ = juce::File::getSpecialLocation(juce::File::userMusicDirectory)
                       .getChildFile("recordings")
                       .getChildFile("rec_" + ts + ".wav")
                       .getFullPathName();
    pathEditor_.setText(currentPath_);
    addAndMakeVisible(pathEditor_);

    browseBtn_.onClick = [this]() { browseForPath(); };
    addAndMakeVisible(browseBtn_);

    // Buttons
    startBtn_.onClick = [this]() {
        currentPath_ = pathEditor_.getText();
        if (onStartRecording) onStartRecording(currentPath_, currentCodec_, currentQuality());
    };
    addAndMakeVisible(startBtn_);

    stopBtn_.onClick = [this]() { if (onStopRecording) onStopRecording(); };
    stopBtn_.setEnabled(false);
    addAndMakeVisible(stopBtn_);

    // Stats
    dataSizeLabelHeader_.setFont(juce::Font(juce::FontOptions(11.0f)));
    dataSizeLabelHeader_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(dataSizeLabelHeader_);
    dataSizeLabel_.setText("0 KB", juce::dontSendNotification);
    dataSizeLabel_.setFont(juce::Font(juce::FontOptions("Courier New", 20.0f, juce::Font::bold)));
    dataSizeLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    addAndMakeVisible(dataSizeLabel_);

    elapsedLabelHeader_.setFont(juce::Font(juce::FontOptions(11.0f)));
    elapsedLabelHeader_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    elapsedLabelHeader_.setText("Elapsed", juce::dontSendNotification);
    addAndMakeVisible(elapsedLabelHeader_);
    elapsedLabel_.setText("00:00", juce::dontSendNotification);
    elapsedLabel_.setFont(juce::Font(juce::FontOptions("Courier New", 20.0f, juce::Font::bold)));
    elapsedLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(elapsedLabel_);

    pathLabel_.setFont(juce::Font(juce::FontOptions(10.0f)));
    pathLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextDim));
    pathLabel_.setText("", juce::dontSendNotification);
    addAndMakeVisible(pathLabel_);
}

void RecordingPanel::updateQualityOptions() {
    qualityBox_.clear();
    if (isLossless(currentCodec_)) {
        qualityLabel_.setText("Bit Depth", juce::dontSendNotification);
        qualityBox_.addItem("16-bit", 16);
        qualityBox_.addItem("24-bit", 24);
        qualityBox_.addItem("32-bit float", 32);
        qualityBox_.setSelectedId(16);
    } else {
        qualityLabel_.setText("Bitrate", juce::dontSendNotification);
        for (int br : {32, 48, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}) {
            qualityBox_.addItem(juce::String(br) + " kbps", br);
        }
        qualityBox_.setSelectedId(128);
    }
}

void RecordingPanel::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kPanel));
    g.fillRoundedRectangle(b, 4.0f);
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
    g.drawRoundedRectangle(b.reduced(0.5f), 4.0f, 1.0f);
}

void RecordingPanel::resized() {
    auto bounds = getLocalBounds().reduced(12, 10);
    headerLabel_.setBounds(bounds.removeFromTop(22));
    bounds.removeFromTop(6);

    // Row 1: Path + Browse
    auto pathRow = bounds.removeFromTop(28);
    browseBtn_.setBounds(pathRow.removeFromRight(36));
    pathRow.removeFromRight(4);
    pathEditor_.setBounds(pathRow);
    bounds.removeFromTop(6);

    // Row 2: Codec + Quality
    auto settingsRow = bounds.removeFromTop(28);
    codecLabel_.setBounds(settingsRow.removeFromLeft(40));
    codecBox_.setBounds(settingsRow.removeFromLeft(80));
    settingsRow.removeFromLeft(12);
    qualityLabel_.setBounds(settingsRow.removeFromLeft(50));
    qualityBox_.setBounds(settingsRow);
    bounds.removeFromTop(6);

    // Row 3: Buttons
    auto btnRow = bounds.removeFromTop(30);
    startBtn_.setBounds(btnRow.removeFromLeft(140).reduced(0, 2));
    btnRow.removeFromLeft(10);
    stopBtn_.setBounds(btnRow.removeFromLeft(140).reduced(0, 2));
    bounds.removeFromTop(8);

    // Row 4: Stats (Size + Elapsed)
    auto statsRow = bounds.removeFromTop(54);
    auto leftStat = statsRow.removeFromLeft(statsRow.getWidth() / 2);
    dataSizeLabelHeader_.setBounds(leftStat.removeFromTop(16));
    dataSizeLabel_.setBounds(leftStat);
    elapsedLabelHeader_.setBounds(statsRow.removeFromTop(16));
    elapsedLabel_.setBounds(statsRow);

    // Row 5: File path display
    bounds.removeFromTop(4);
    pathLabel_.setBounds(bounds.removeFromTop(16));
}

void RecordingPanel::setRecordingActive(bool active) {
    startBtn_.setEnabled(!active);
    stopBtn_.setEnabled(active);
    codecBox_.setEnabled(!active);
    qualityBox_.setEnabled(!active);
    pathEditor_.setEnabled(!active);
    browseBtn_.setEnabled(!active);

    if (!active) {
        dataSizeLabel_.setText("0 KB", juce::dontSendNotification);
        elapsedLabel_.setText("00:00", juce::dontSendNotification);
        pathLabel_.setText("", juce::dontSendNotification);
    }
}

void RecordingPanel::setRecordingSize(const juce::String& size) {
    dataSizeLabel_.setText(size, juce::dontSendNotification);
}

void RecordingPanel::setRecordingElapsed(const juce::String& elapsed) {
    elapsedLabel_.setText(elapsed, juce::dontSendNotification);
}

void RecordingPanel::setRecordingPath(const juce::String& path) {
    currentPath_ = path;
    pathLabel_.setText("Recording: " + path, juce::dontSendNotification);
}

void RecordingPanel::browseForPath() {
    juce::FileChooser chooser("Save recording as...",
        juce::File(currentPath_),
        "*.wav;*.flac;*.aiff;*.mp3;*.opus;*.mp2");
    if (chooser.browseForFileToSave(true)) {
        currentPath_ = chooser.getResult().getFullPathName();
        pathEditor_.setText(currentPath_);
    }
}

} // namespace ore
