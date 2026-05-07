#include "MixerDialog.h"
#include "ui/LookAndFeel_OpenRadio.h"
#include <algorithm>
#include <cstdio>

namespace ore {

MixerDialog::MixerDialog(AudioPipeline* pipeline)
    : pipeline_(pipeline) {

    header_.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    header_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    addAndMakeVisible(header_);

    deviceLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    deviceLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(deviceLabel_);

    deviceName_.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    deviceName_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentBlue));
    if (pipeline_ && pipeline_->config().deviceIndex >= 0) {
        auto devices = AudioPipeline::getAvailableDevices();
        for (auto& d : devices) {
            if (d.id == pipeline_->config().deviceIndex) {
                deviceName_.setText(d.name, juce::dontSendNotification);
                break;
            }
        }
    }
    if (deviceName_.getText().isEmpty())
        deviceName_.setText("Default System Input", juce::dontSendNotification);
    addAndMakeVisible(deviceName_);

    leftLabel_.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    leftLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    leftLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(leftLabel_);

    rightLabel_.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    rightLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
    rightLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(rightLabel_);

    leftDb_.setFont(juce::Font(juce::FontOptions("Courier New", 28.0f, juce::Font::bold)));
    leftDb_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kVUGreen));
    leftDb_.setJustificationType(juce::Justification::centred);
    leftDb_.setText("-inf dB", juce::dontSendNotification);
    addAndMakeVisible(leftDb_);

    rightDb_.setFont(juce::Font(juce::FontOptions("Courier New", 28.0f, juce::Font::bold)));
    rightDb_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kVUGreen));
    rightDb_.setJustificationType(juce::Justification::centred);
    rightDb_.setText("-inf dB", juce::dontSendNotification);
    addAndMakeVisible(rightDb_);

    leftPeak_.setFont(juce::Font(juce::FontOptions(10.0f)));
    leftPeak_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextDim));
    leftPeak_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(leftPeak_);

    rightPeak_.setFont(juce::Font(juce::FontOptions(10.0f)));
    rightPeak_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextDim));
    rightPeak_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(rightPeak_);

    masterLabel_.setFont(juce::Font(juce::FontOptions(11.0f)));
    masterLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    addAndMakeVisible(masterLabel_);

    masterSlider_.setRange(0.0, 2.0, 0.01);
    masterSlider_.setValue(1.0);
    masterSlider_.setSliderStyle(juce::Slider::LinearHorizontal);
    masterSlider_.setTextBoxStyle(juce::Slider::TextBoxRight, false, 55, 20);
    masterSlider_.onValueChange = [this]() {
        float gain = static_cast<float>(masterSlider_.getValue());
        if (pipeline_) pipeline_->setMasterGain(gain);
        char buf[8];
        snprintf(buf, sizeof(buf), "%.2f", gain);
        masterValue_.setText(buf, juce::dontSendNotification);
    };
    addAndMakeVisible(masterSlider_);

    masterValue_.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    masterValue_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    masterValue_.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(masterValue_);

    setSize(540, 460);
    startTimerHz(60);
}

MixerDialog::~MixerDialog() {
    stopTimer();
}

void MixerDialog::paint(juce::Graphics& g) {
    auto b = getLocalBounds().toFloat();
    g.fillAll(juce::Colour(LookAndFeel_OpenRadio::kBackground));

    // VU meters area
    float meterTop = b.getY() + 100.0f;
    float meterBottom = b.getY() + 340.0f;
    float meterHeight = meterBottom - meterTop;
    float barWidth = 28.0f;
    float centerX = b.getCentreX();

    // Left meter area
    float leftBarX = centerX - barWidth - 30.0f;
    float rightBarX = centerX + 30.0f;

    // Draw dB scale on left side
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kTextDim));
    for (int db : {0, -3, -6, -12, -18, -24, -30, -36, -45, -60}) {
        float pos = juce::jmap(static_cast<float>(db), -60.0f, 0.0f, 0.0f, meterHeight);
        float y = meterBottom - pos;
        g.drawText(juce::String(db), b.getX() + 10.0f, y - 6.0f, 24.0f, 12.0f,
                   juce::Justification::centredRight, false);
    }

    // Draw meter backgrounds
    g.setColour(juce::Colour(0xFF0A0A14));
    g.fillRect(leftBarX, meterTop, barWidth, meterHeight);
    g.fillRect(rightBarX, meterTop, barWidth, meterHeight);

    // Draw meter bars
    for (int side = 0; side < 2; side++) {
        float levelDb = (side == 0) ? peakL_ : peakR_;
        float barX = (side == 0) ? leftBarX : rightBarX;
        float peakDb = (side == 0) ? peakHoldL_ : peakHoldR_;

        float levelY = juce::jmap(levelDb, -60.0f, 0.0f, 0.0f, meterHeight);
        levelY = std::clamp(levelY, 0.0f, meterHeight);

        int fillPixels = static_cast<int>(levelY);
        for (int i = 0; i < fillPixels; i++) {
            float py = meterBottom - static_cast<float>(i);
            float segPos = static_cast<float>(i) / meterHeight;
            juce::Colour col;
            if (segPos > 0.9f) col = juce::Colour(LookAndFeel_OpenRadio::kVURed);
            else if (segPos > 0.8f) col = juce::Colour(LookAndFeel_OpenRadio::kVUYellow);
            else col = juce::Colour(LookAndFeel_OpenRadio::kVUGreen);
            g.setColour(col);
            g.drawHorizontalLine(py, barX + 2, barX + barWidth - 2);
        }

        // Peak hold line (white)
        if (peakDb > -60.0f) {
            float peakY = juce::jmap(peakDb, -60.0f, 0.0f, 0.0f, meterHeight);
            peakY = std::clamp(peakY, 0.0f, meterHeight);
            g.setColour(juce::Colours::white.withAlpha(0.85f));
            g.drawHorizontalLine(meterBottom - peakY, barX, barX + barWidth);
        }
    }

    // Meter outer borders
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
    g.drawRect(leftBarX, meterTop, barWidth, meterHeight, 1.0f);
    g.drawRect(rightBarX, meterTop, barWidth, meterHeight, 1.0f);

    // Zone labels
    g.setFont(juce::Font(juce::FontOptions(8.0f)));
    float zoneY = meterTop;
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kVURed));
    g.drawText("CLIP", leftBarX, zoneY + 2, barWidth, 12, juce::Justification::centred, false);
    g.drawText("CLIP", rightBarX, zoneY + 2, barWidth, 12, juce::Justification::centred, false);

    // RMS text inside meters
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    char rmsBuf[16];
    snprintf(rmsBuf, sizeof(rmsBuf), "%+.0f", rmsL_);
    g.drawText(juce::String("RMS ") + juce::String(rmsBuf), leftBarX, meterBottom - 18, barWidth, 14,
               juce::Justification::centred, false);
    snprintf(rmsBuf, sizeof(rmsBuf), "%+.0f", rmsR_);
    g.drawText(juce::String("RMS ") + juce::String(rmsBuf), rightBarX, meterBottom - 18, barWidth, 14,
               juce::Justification::centred, false);
}

void MixerDialog::resized() {
    auto b = getLocalBounds().reduced(15);

    header_.setBounds(b.removeFromTop(28));
    b.removeFromTop(2);

    deviceLabel_.setBounds(b.removeFromTop(16));
    deviceName_.setBounds(b.removeFromTop(22));
    b.removeFromTop(8);

    auto centerX = getWidth() / 2;
    float barWidth = 28.0f;

    leftLabel_.setBounds(centerX - barWidth - 30, 84, barWidth, 16);
    rightLabel_.setBounds(centerX + 30, 84, barWidth, 16);

    float meterBottom = 340;
    leftDb_.setBounds(centerX - barWidth - 60, meterBottom + 4, barWidth * 2 + 20, 30);
    rightDb_.setBounds(centerX - barWidth, meterBottom + 4, barWidth * 2 + 20, 30);
    leftPeak_.setBounds(centerX - barWidth - 60, meterBottom + 36, barWidth * 2 + 20, 14);
    rightPeak_.setBounds(centerX - barWidth, meterBottom + 36, barWidth * 2 + 20, 14);

    auto gainArea = getLocalBounds().removeFromBottom(70).reduced(20, 0);
    masterLabel_.setBounds(gainArea.removeFromTop(16));
    auto gainRow = gainArea.removeFromTop(32);
    masterValue_.setBounds(gainRow.removeFromRight(55));
    gainRow.removeFromRight(6);
    masterSlider_.setBounds(gainRow.reduced(0, 2));
}

void MixerDialog::timerCallback() {
    if (!pipeline_) return;

    auto& vu = pipeline_->vuMeterData();
    peakL_ = vu.peakL.load();
    peakR_ = vu.peakR.load();
    rmsL_ = vu.rmsL.load();
    rmsR_ = vu.rmsR.load();

    // Peak hold with decay
    if (peakL_ > peakHoldL_) {
        peakHoldL_ = peakL_;
        peakHoldCounterL_ = kPeakHoldFrames;
    } else if (peakHoldCounterL_ > 0) {
        peakHoldCounterL_--;
    } else if (peakHoldL_ > -60.0f) {
        peakHoldL_ -= kPeakDecay;
    }

    if (peakR_ > peakHoldR_) {
        peakHoldR_ = peakR_;
        peakHoldCounterR_ = kPeakHoldFrames;
    } else if (peakHoldCounterR_ > 0) {
        peakHoldCounterR_--;
    } else if (peakHoldR_ > -60.0f) {
        peakHoldR_ -= kPeakDecay;
    }

    char buf[16];
    snprintf(buf, sizeof(buf), "%.1f dB", peakL_);
    leftDb_.setText(buf, juce::dontSendNotification);
    snprintf(buf, sizeof(buf), "%.1f dB", peakR_);
    rightDb_.setText(buf, juce::dontSendNotification);

    snprintf(buf, sizeof(buf), "Peak %.1f", peakHoldL_);
    leftPeak_.setText(buf, juce::dontSendNotification);
    snprintf(buf, sizeof(buf), "Peak %.1f", peakHoldR_);
    rightPeak_.setText(buf, juce::dontSendNotification);

    auto colorFor = [](float db) -> juce::Colour {
        if (db > -3.0f) return juce::Colour(LookAndFeel_OpenRadio::kVURed);
        if (db > -10.0f) return juce::Colour(LookAndFeel_OpenRadio::kVUYellow);
        return juce::Colour(LookAndFeel_OpenRadio::kVUGreen);
    };
    leftDb_.setColour(juce::Label::textColourId, colorFor(peakL_));
    rightDb_.setColour(juce::Label::textColourId, colorFor(peakR_));

    repaint();
}

} // namespace ore
