#include "SettingsDialog.h"
#include "ui/LookAndFeel_OpenRadio.h"

namespace ore {

SettingsDialog::SettingsDialog(AudioPipeline* pipeline)
    : pipeline_(pipeline) {

    header_.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    addAndMakeVisible(header_);

    audioGroup_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    addAndMakeVisible(audioGroup_);

    deviceLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(deviceLabel_);
    deviceCombo_.addItem("Default Device", 1);
    for (auto& dev : AudioPipeline::getAvailableDevices()) {
        deviceCombo_.addItem(juce::String(dev.name) + " (" + juce::String(dev.maxInputChannels) + "ch)", dev.id + 2);
    }
    
    if (pipeline_) {
        int devIdx = pipeline_->config().deviceIndex;
        deviceCombo_.setSelectedId(devIdx == -1 ? 1 : devIdx + 2);
    } else {
        deviceCombo_.setSelectedId(1);
    }
    addAndMakeVisible(deviceCombo_);

    sampleRateLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(sampleRateLabel_);
    for (auto rate : {22050, 44100, 48000, 96000}) {
        sampleRateCombo_.addItem(juce::String(rate) + " Hz", rate);
    }
    if (pipeline_) sampleRateCombo_.setSelectedId(pipeline_->config().sampleRate);
    else sampleRateCombo_.setSelectedId(44100);
    addAndMakeVisible(sampleRateCombo_);

    bufferSizeLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(bufferSizeLabel_);
    for (auto bs : {256, 512, 1024, 2048, 4096}) {
        bufferSizeCombo_.addItem(juce::String(bs) + " frames", bs);
    }
    if (pipeline_) bufferSizeCombo_.setSelectedId(pipeline_->config().bufferSize);
    else bufferSizeCombo_.setSelectedId(1024);
    addAndMakeVisible(bufferSizeCombo_);

    recordingGroup_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    addAndMakeVisible(recordingGroup_);

    recordingPathLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(recordingPathLabel_);
    recordingPathEditor_.setText(juce::File::getSpecialLocation(
        juce::File::userMusicDirectory).getFullPathName() + "/recordings/");
    addAndMakeVisible(recordingPathEditor_);
    browseBtn_.onClick = [this]() {
        juce::FileChooser chooser("Select Recording Output Folder",
            juce::File(recordingPathEditor_.getText()),
            "*");
        chooser.browseForDirectory();
        auto result = chooser.getResult();
        if (result != juce::File()) {
            recordingPathEditor_.setText(result.getFullPathName() + "/");
        }
    };
    addAndMakeVisible(browseBtn_);

    applyBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    applyBtn_.onClick = [this]() { applySettings(); };
    addAndMakeVisible(applyBtn_);

    cancelBtn_.onClick = [this]() {
        statusLabel_.setText("Settings reverted", juce::dontSendNotification);
    };
    addAndMakeVisible(cancelBtn_);

    statusLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    statusLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextDim));
    statusLabel_.setJustificationType(juce::Justification::centred);
    statusLabel_.setText("Click Apply to restart audio pipeline with new settings", juce::dontSendNotification);
    addAndMakeVisible(statusLabel_);

    setSize(540, 420);
}

void SettingsDialog::applySettings() {
    if (!pipeline_) {
        statusLabel_.setText("Pipeline not available", juce::dontSendNotification);
        return;
    }

    PipelineConfig cfg;
    cfg.sampleRate = sampleRateCombo_.getSelectedId();
    cfg.channels = 2;
    cfg.bufferSize = bufferSizeCombo_.getSelectedId();
    
    int selectedDevId = deviceCombo_.getSelectedId();
    cfg.deviceIndex = (selectedDevId == 1) ? -1 : (selectedDevId - 2);

    int rc = pipeline_->restart(cfg);
    if (rc == 0) {
        statusLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
        statusLabel_.setText("Settings applied successfully", juce::dontSendNotification);

        if (onSettingsChanged) onSettingsChanged();
    } else {
        statusLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentRed));
        statusLabel_.setText("Failed to apply settings. Pipeline restarted with previous config.", juce::dontSendNotification);
    }
}

void SettingsDialog::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(LookAndFeel_OpenRadio::kBackground));

    auto b = getLocalBounds().toFloat().reduced(15.0f);
    float groupY = b.getY() + 30.0f;

    auto drawGroup = [&](juce::Label& groupLabel, float height) {
        float x = b.getX() + 10.0f;
        float w = b.getWidth() - 20.0f;
        g.setColour(juce::Colour(LookAndFeel_OpenRadio::kSurface));
        g.fillRoundedRectangle(x, groupY, w, height, 6.0f);
        g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
        g.drawRoundedRectangle(x, groupY, w, height, 6.0f, 1.0f);
    };
}

void SettingsDialog::resized() {
    auto b = getLocalBounds().reduced(20, 15);
    header_.setBounds(b.removeFromTop(28));
    b.removeFromTop(10);

    auto makeRow = [&](juce::Label& label, juce::Component& comp, int h = 28) {
        label.setBounds(b.removeFromTop(16));
        comp.setBounds(b.removeFromTop(h));
        b.removeFromTop(6);
    };

    audioGroup_.setBounds(b.removeFromTop(22));
    b.removeFromTop(6);

    makeRow(deviceLabel_, deviceCombo_);
    makeRow(sampleRateLabel_, sampleRateCombo_);
    makeRow(bufferSizeLabel_, bufferSizeCombo_);

    b.removeFromTop(8);
    recordingGroup_.setBounds(b.removeFromTop(22));
    b.removeFromTop(6);

    recordingPathLabel_.setBounds(b.removeFromTop(16));
    auto row = b.removeFromTop(28);
    browseBtn_.setBounds(row.removeFromRight(80));
    row.removeFromRight(6);
    recordingPathEditor_.setBounds(row);

    b.removeFromTop(12);

    auto btnRow = b.removeFromTop(34);
    cancelBtn_.setBounds(btnRow.removeFromRight(120));
    btnRow.removeFromRight(8);
    applyBtn_.setBounds(btnRow.removeFromRight(180));
    btnRow.removeFromRight(8);

    b.removeFromTop(6);
    statusLabel_.setBounds(b.removeFromTop(18));
}

} // namespace ore
