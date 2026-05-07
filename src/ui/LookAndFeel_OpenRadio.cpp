// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Dark Mode Look & Feel Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "LookAndFeel_OpenRadio.h"

namespace ore {

LookAndFeel_OpenRadio::LookAndFeel_OpenRadio() {
    // Set the overall colour scheme
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(kBackground));

    // Text editor
    setColour(juce::TextEditor::backgroundColourId, juce::Colour(kSurface));
    setColour(juce::TextEditor::textColourId, juce::Colour(kTextPrimary));
    setColour(juce::TextEditor::outlineColourId, juce::Colour(kBorder));
    setColour(juce::TextEditor::focusedOutlineColourId, juce::Colour(kAccentBlue));

    // Labels
    setColour(juce::Label::textColourId, juce::Colour(kTextPrimary));

    // Buttons
    setColour(juce::TextButton::buttonColourId, juce::Colour(kButtonFace));
    setColour(juce::TextButton::textColourOffId, juce::Colour(kButtonText));
    setColour(juce::TextButton::textColourOnId, juce::Colour(kTextPrimary));

    // Toggle buttons
    setColour(juce::ToggleButton::textColourId, juce::Colour(kTextPrimary));
    setColour(juce::ToggleButton::tickColourId, juce::Colour(kAccentGreen));

    // Combo box
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(kSurface));
    setColour(juce::ComboBox::textColourId, juce::Colour(kTextPrimary));
    setColour(juce::ComboBox::outlineColourId, juce::Colour(kBorder));

    // Scrollbar
    setColour(juce::ScrollBar::thumbColourId, juce::Colour(kBorder));

    // Group component
    setColour(juce::GroupComponent::outlineColourId, juce::Colour(kBorder));
    setColour(juce::GroupComponent::textColourId, juce::Colour(kTextPrimary));

    // List box
    setColour(juce::ListBox::backgroundColourId, juce::Colour(kSurface));
    setColour(juce::ListBox::textColourId, juce::Colour(kTextPrimary));

    // Slider colours
    setColour(juce::Slider::thumbColourId, juce::Colour(kAccentBlue));
    setColour(juce::Slider::trackColourId, juce::Colour(kAccentGreen));
    setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(kAccentBlue));
    setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(kBorder));
    setColour(juce::Slider::backgroundColourId, juce::Colour(kSurface));
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(kTextPrimary));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(kSurface));
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(kBorder));
}

juce::Font LookAndFeel_OpenRadio::getDefaultFont() const {
    return juce::Font(juce::FontOptions(16.0f));
}

juce::Font LookAndFeel_OpenRadio::getComboBoxFont(juce::ComboBox&) {
    return getDefaultFont();
}

juce::Font LookAndFeel_OpenRadio::getLabelFont(juce::Label&) {
    return getDefaultFont();
}

void LookAndFeel_OpenRadio::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& /*bgColour*/,
                                                  bool highlighted, bool down) {
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f);
    auto cornerSize = 4.0f;

    juce::Colour bg = juce::Colour(kButtonFace);
    if (down)
        bg = bg.brighter(0.15f);
    else if (highlighted)
        bg = juce::Colour(kSurfaceHover);

    g.setColour(bg);
    g.fillRoundedRectangle(bounds, cornerSize);

    g.setColour(juce::Colour(kBorder));
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

void LookAndFeel_OpenRadio::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                            bool /*highlighted*/, bool /*down*/) {
    auto font = getDefaultFont();
    g.setFont(font);
    g.setColour(button.findColour(button.getToggleState()
        ? juce::TextButton::textColourOnId
        : juce::TextButton::textColourOffId));

    auto area = button.getLocalBounds();
    g.drawText(button.getButtonText(), area, juce::Justification::centred, true);
}

void LookAndFeel_OpenRadio::drawGroupComponentOutline(juce::Graphics& g, int w, int h,
                                                       const juce::String& text,
                                                       const juce::Justification& /*pos*/,
                                                       juce::GroupComponent& /*group*/) {
    auto font = getDefaultFont().withHeight(14.0f);
    juce::GlyphArrangement ga;
    ga.addLineOfText(font, text, 0.0f, 0.0f);
    auto textW = ga.getBoundingBox(0, text.length(), true).getWidth() + 8;
    auto indent = 10;
    auto textY = 0;

    g.setFont(font);
    g.setColour(juce::Colour(kTextPrimary));
    g.drawText(text, indent, textY, textW, 20, juce::Justification::centredLeft, true);

    auto lineY = 10.0f;
    g.setColour(juce::Colour(kBorder));
    g.drawLine(0, lineY, 6, lineY);
    g.drawLine(float(indent + textW + 2), lineY, float(w), lineY);
    g.drawLine(0, lineY, 0, float(h));
    g.drawLine(float(w) - 1, lineY, float(w) - 1, float(h));
    g.drawLine(0, float(h) - 1, float(w), float(h) - 1);
}

void LookAndFeel_OpenRadio::drawTextEditorOutline(juce::Graphics& g, int w, int h,
                                                   juce::TextEditor& editor) {
    auto bounds = juce::Rectangle<float>(0, 0, (float)w, (float)h);
    g.setColour(editor.hasKeyboardFocus(true) ? juce::Colour(kAccentBlue) : juce::Colour(kBorder));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 4.0f, 1.0f);
}

void LookAndFeel_OpenRadio::fillTextEditorBackground(juce::Graphics& g, int w, int h,
                                                      juce::TextEditor& /*editor*/) {
    g.setColour(juce::Colour(kSurface));
    g.fillRoundedRectangle(0.0f, 0.0f, (float)w, (float)h, 4.0f);
}

} // namespace ore
