#pragma once
#include <JuceHeader.h>
#include "Icons.h"

namespace ore {

class ToolbarButton : public juce::Button {
public:
    ToolbarButton(const juce::String& label, juce::Path icon)
        : Button(label), label_(label), iconPath_(std::move(icon)) {}

    void setSelected(bool s) { selected_ = s; repaint(); }

    void paintButton(juce::Graphics& g, bool highlighted, bool down) override {
        auto bounds = getLocalBounds().toFloat();

        juce::Colour bg = juce::Colour(0xFF1A1A2A);
        if (selected_ || down) bg = juce::Colour(0xFF2A2A4A);
        else if (highlighted) bg = bg.brighter(0.08f);

        g.setColour(bg);
        g.fillRoundedRectangle(bounds.reduced(1), 3.0f);

        // Active indicator bar at bottom
        if (selected_) {
            g.setColour(juce::Colour(0xFF4A9EFF));
            g.fillRoundedRectangle(bounds.getX() + 6, bounds.getBottom() - 3,
                                   bounds.getWidth() - 12, 3.0f, 1.0f);
        }

        auto iconArea = bounds.removeFromTop(bounds.getHeight() * 0.55f);
        juce::Colour iconColour = (selected_ || highlighted) ? juce::Colour(0xFFE0E0E0)
                                                              : juce::Colour(0xFF9090A0);
        Icons::drawIcon(g, iconPath_, iconArea, iconColour, 5.0f);

        g.setFont(juce::Font(juce::FontOptions("Inter", 9.0f, juce::Font::plain)));
        g.setColour(selected_ ? juce::Colour(0xFFE0E0E0) : juce::Colour(0xFF9090A0));
        g.drawText(label_, bounds, juce::Justification::centred, false);
    }

private:
    juce::String label_;
    juce::Path iconPath_;
    bool selected_ = false;
};

class ToolbarComponent : public juce::Component {
public:
    ToolbarComponent();
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int)> onButtonClicked;
    void setSelectedTab(int index);
    void setCpuUsage(float percent);

private:
    std::vector<std::unique_ptr<ToolbarButton>> buttons_;
    int selectedIndex_ = 0;
    juce::Label cpuLabel_;

    void addButton(const juce::String& label, juce::Path icon);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ToolbarComponent)
};

} // namespace ore
