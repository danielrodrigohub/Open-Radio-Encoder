// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Dark Mode Look & Feel
// Custom JUCE LookAndFeel matching the BUTTM 1.0.0 Pro aesthetic
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include <JuceHeader.h>

namespace ore {

class LookAndFeel_OpenRadio : public juce::LookAndFeel_V4 {
public:
    // ── Color Palette (extracted from BUTTM reference image) ──
    static constexpr uint32_t kBackground       = 0xFF1E1E2E;   // Deep dark blue-gray
    static constexpr uint32_t kSurface           = 0xFF2A2A3C;   // Slightly lighter surface
    static constexpr uint32_t kSurfaceHover      = 0xFF353548;   // Hover state
    static constexpr uint32_t kPanel             = 0xFF252536;   // Panel backgrounds
    static constexpr uint32_t kBorder            = 0xFF3A3A4C;   // Subtle borders
    static constexpr uint32_t kTextPrimary       = 0xFFE0E0E0;   // Primary text
    static constexpr uint32_t kTextSecondary     = 0xFF9090A0;   // Secondary text
    static constexpr uint32_t kTextDim           = 0xFF606072;   // Dimmed text
    static constexpr uint32_t kAccentGreen       = 0xFF00FF00;   // Stream time, recording size
    static constexpr uint32_t kAccentRed         = 0xFFFF3B3B;   // Disconnect, errors
    static constexpr uint32_t kAccentBlue        = 0xFF4A9EFF;   // Focused elements
    static constexpr uint32_t kButtonFace        = 0xFF3A3A4E;   // Button background
    static constexpr uint32_t kButtonText        = 0xFFD0D0D0;   // Button text
    static constexpr uint32_t kToolbarBg         = 0xFF1A1A2A;   // Toolbar background
    static constexpr uint32_t kVUGreen           = 0xFF00CC00;   // VU meter green
    static constexpr uint32_t kVUYellow          = 0xFFCCCC00;   // VU meter yellow
    static constexpr uint32_t kVURed             = 0xFFCC0000;   // VU meter red

    LookAndFeel_OpenRadio();

    // ── Font ──
    juce::Font getDefaultFont() const;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;

    // ── Overrides ──
    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                        bool shouldDrawButtonAsHighlighted,
                        bool shouldDrawButtonAsDown) override;

    void drawGroupComponentOutline(juce::Graphics& g, int width, int height,
                                   const juce::String& text,
                                   const juce::Justification& position,
                                   juce::GroupComponent& group) override;

    void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                               juce::TextEditor& editor) override;

    void fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                  juce::TextEditor& editor) override;
};

} // namespace ore
