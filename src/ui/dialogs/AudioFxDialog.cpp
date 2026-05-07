#include "AudioFxDialog.h"
#include "ui/LookAndFeel_OpenRadio.h"
#include "dsp/dsp_effects.h"
#include <cstdio>

namespace ore {

const char* AudioFxDialog::kBandNames[] = {
    "31Hz", "62Hz", "125Hz", "250Hz", "500Hz",
    "1kHz", "2kHz", "4kHz", "8kHz", "16kHz"
};

AudioFxDialog::AudioFxDialog(AudioPipeline* pipeline)
    : pipeline_(pipeline), dsp_(pipeline ? pipeline->dsp() : nullptr) {

    eqHeader_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    addAndMakeVisible(eqHeader_);

    eqBypass_.onClick = [this]() {
        if (dsp_) dsp_->setEQActive(eqBypass_.getToggleState());
    };
    addAndMakeVisible(eqBypass_);

    for (int i = 0; i < kBandCount; i++) {
        eqSliders_[i].setRange(-20.0, 20.0, 0.5);
        eqSliders_[i].setValue(0.0);
        eqSliders_[i].setSliderStyle(juce::Slider::LinearVertical);
        eqSliders_[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 40, 16);
        eqSliders_[i].onValueChange = [this, i]() {
            if (dsp_) dsp_->setEQband(i, eqSliders_[i].getValue());
        };
        addAndMakeVisible(eqSliders_[i]);

        eqLabels_[i].setText(kBandNames[i], juce::dontSendNotification);
        eqLabels_[i].setFont(juce::Font(juce::FontOptions(10.0f)));
        eqLabels_[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(eqLabels_[i]);
    }

    // Compressor section
    compHeader_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    addAndMakeVisible(compHeader_);

    compBypass_.onClick = [this]() {
        if (dsp_) dsp_->setCompressorActive(compBypass_.getToggleState());
    };
    addAndMakeVisible(compBypass_);

    auto setupCompSlider = [this](juce::Slider& s, juce::Label& l,
                                   const juce::String& name, double min, double max, double def,
                                   double step = 0.5) {
        s.setRange(min, max, step);
        s.setValue(def);
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 16);
        addAndMakeVisible(s);
        l.setText(name, juce::dontSendNotification);
        l.setFont(juce::Font(juce::FontOptions(10.0f)));
        l.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(l);
    };

    setupCompSlider(thresholdSlider_, thresholdLabel_, "Threshold (dB)", -60.0, 0.0, -20.0);
    thresholdSlider_.onValueChange = [this]() { /* TODO: set via API */ };
    setupCompSlider(ratioSlider_, ratioLabel_, "Ratio (:1)", 1.0, 20.0, 4.0);
    setupCompSlider(attackSlider_, attackLabel_, "Attack (ms)", 0.1, 100.0, 10.0, 0.1);
    setupCompSlider(releaseSlider_, releaseLabel_, "Release (ms)", 1.0, 1000.0, 100.0, 1.0);
    setupCompSlider(makeupSlider_, makeupLabel_, "Makeup (dB)", 0.0, 24.0, 0.0);

    setSize(600, 380);
}

void AudioFxDialog::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(LookAndFeel_OpenRadio::kBackground));
}

void AudioFxDialog::resized() {
    auto b = getLocalBounds().reduced(10);

    // EQ section
    eqHeader_.setBounds(b.removeFromTop(22));
    eqBypass_.setBounds(b.removeFromTop(24).removeFromRight(100));

    auto eqArea = b.removeFromTop(200);
    int sliderW = eqArea.getWidth() / kBandCount;
    for (int i = 0; i < kBandCount; i++) {
        auto col = eqArea.removeFromLeft(sliderW).reduced(3);
        eqSliders_[i].setBounds(col.removeFromTop(col.getHeight() - 16));
        eqLabels_[i].setBounds(col);
    }

    b.removeFromTop(10);

    // Compressor section
    compHeader_.setBounds(b.removeFromTop(22));
    compBypass_.setBounds(b.removeFromTop(24).removeFromRight(120));

    auto compRow = b.removeFromTop(70);
    int knobW = compRow.getWidth() / 5;
    for (auto* s : {&thresholdSlider_, &ratioSlider_, &attackSlider_, &releaseSlider_, &makeupSlider_}) {
        s->setBounds(compRow.removeFromLeft(knobW).reduced(5));
    }
}

} // namespace ore
