#include "ToolbarComponent.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

ToolbarComponent::ToolbarComponent() {
    addButton("Streaming",   Icons::satellite());
    addButton("Settings",    Icons::gear());
    addButton("Scheduler",   Icons::calendar());
    addButton("Station Mgr", Icons::server());
    addButton("Console",    Icons::music());
    addButton("Audio FX",    Icons::audioFx());
    addButton("Mixer",       Icons::mixer());

    cpuLabel_.setFont(juce::Font(juce::FontOptions("Courier New", 12.0f, juce::Font::bold)));
    cpuLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    cpuLabel_.setText("CPU: 0.0%", juce::dontSendNotification);
    addAndMakeVisible(cpuLabel_);
}

void ToolbarComponent::addButton(const juce::String& label, juce::Path icon) {
    auto btn = std::make_unique<ToolbarButton>(label, std::move(icon));
    int index = static_cast<int>(buttons_.size());
    btn->onClick = [this, index]() {
        setSelectedTab(index);
        if (onButtonClicked) onButtonClicked(index);
    };
    addAndMakeVisible(btn.get());
    buttons_.push_back(std::move(btn));
}

void ToolbarComponent::setSelectedTab(int index) {
    selectedIndex_ = index;
    for (int i = 0; i < static_cast<int>(buttons_.size()); i++) {
        buttons_[static_cast<size_t>(i)]->setSelected(i == index);
    }
}

void ToolbarComponent::setCpuUsage(float percent) {
    cpuLabel_.setText("CPU: " + juce::String(percent, 1) + "%", juce::dontSendNotification);
    if (percent > 80.0f) cpuLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentRed));
    else if (percent > 50.0f) cpuLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kVUYellow));
    else cpuLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
}

void ToolbarComponent::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kToolbarBg));
    g.fillRect(getLocalBounds());

    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
    g.drawLine(0, (float)getHeight() - 1, (float)getWidth(), (float)getHeight() - 1);
}

void ToolbarComponent::resized() {
    auto bounds = getLocalBounds().reduced(4, 4);
    
    // Reserve space for CPU label on the right
    auto cpuArea = bounds.removeFromRight(100);
    cpuLabel_.setBounds(cpuArea.withSize(100, 20).withCentre(cpuArea.getCentre()));

    int count = static_cast<int>(buttons_.size());
    if (count == 0) return;
    int buttonWidth = bounds.getWidth() / count;

    for (auto& btn : buttons_) {
        btn->setBounds(bounds.removeFromLeft(buttonWidth).reduced(2));
    }
}

} // namespace ore
