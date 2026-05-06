// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Toolbar Component Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "ToolbarComponent.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

ToolbarComponent::ToolbarComponent() {
    // Match the BUTTM toolbar: Settings, Scheduler, Station Manager,
    // Server Manager, Encoder Manager, Audio Effects, Audio Mixer
    addToolbarButton("Settings",        juce::String::charToString(0x2699));  // ⚙
    addToolbarButton("Scheduler",       juce::String::charToString(0x1F4C5)); // 📅
    addToolbarButton("Station Mgr",     juce::String::charToString(0x1F4E1)); // 📡
    addToolbarButton("Server Mgr",      juce::String::charToString(0x1F5A5)); // 🖥
    addToolbarButton("Encoder Mgr",     juce::String::charToString(0x1F3B5)); // 🎵
    addToolbarButton("Audio FX",        juce::String::charToString(0x2728));  // ✨
    addToolbarButton("Audio Mixer",     juce::String::charToString(0x1F39A)); // 🎚
}

void ToolbarComponent::addToolbarButton(const juce::String& label, const juce::String& icon) {
    ToolbarButton tb;
    tb.label = label;
    tb.icon = icon;
    tb.button = std::make_unique<juce::TextButton>(icon + "\n" + label);
    tb.button->setColour(juce::TextButton::buttonColourId,
                         juce::Colour(LookAndFeel_OpenRadio::kToolbarBg));
    addAndMakeVisible(tb.button.get());
    buttons_.push_back(std::move(tb));
}

void ToolbarComponent::paint(juce::Graphics& g) {
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kToolbarBg));
    g.fillRect(getLocalBounds());

    // Bottom separator line
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
    g.drawLine(0, (float)getHeight() - 1, (float)getWidth(), (float)getHeight() - 1);
}

void ToolbarComponent::resized() {
    auto bounds = getLocalBounds().reduced(5);
    int buttonWidth = bounds.getWidth() / static_cast<int>(buttons_.size());

    for (auto& tb : buttons_) {
        tb.button->setBounds(bounds.removeFromLeft(buttonWidth).reduced(2));
    }
}

} // namespace ore
