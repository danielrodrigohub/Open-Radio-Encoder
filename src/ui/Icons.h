// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Professional Vector Icons
// All icons drawn with juce::Path at 24x24, scalable to any size.
// ═══════════════════════════════════════════════════════════════════════
#pragma once
#include <JuceHeader.h>

namespace ore {
namespace Icons {

static inline juce::Path gear() {
    juce::Path result;
    // Outer ring
    result.addCentredArc(12, 12, 8, 8, 0, 0, juce::MathConstants<float>::twoPi, true);
    result.addCentredArc(12, 12, 5.5f, 5.5f, 0, 0, juce::MathConstants<float>::twoPi, false);
    // Teeth
    for (int i = 0; i < 8; i++) {
        float angle = i * juce::MathConstants<float>::pi / 4.0f;
        float cx = 12 + 7.0f * std::cos(angle);
        float cy = 12 + 7.0f * std::sin(angle);
        result.addRectangle(cx - 1.5f, cy - 1.5f, 3.0f, 3.0f);
    }
    // Center hole
    result.addCentredArc(12, 12, 2.5f, 2.5f, 0, 0, juce::MathConstants<float>::twoPi, false);
    result.setUsingNonZeroWinding(false);
    return result;
}

static inline juce::Path calendar() {
    juce::Path p;
    p.addRoundedRectangle(3, 5, 18, 16, 2);
    p.addRectangle(3, 5, 18, 4);
    p.addRectangle(7, 3, 2, 4);
    p.addRectangle(15, 3, 2, 4);
    p.addRectangle(6, 12, 2, 2);
    p.addRectangle(11, 12, 2, 2);
    p.addRectangle(16, 12, 2, 2);
    p.addRectangle(6, 16, 2, 2);
    p.addRectangle(11, 16, 2, 2);
    p.addRectangle(16, 16, 2, 2);
    return p;
}

static inline juce::Path satellite() {
    juce::Path p;
    // Dish
    p.addCentredArc(10, 14, 7, 7, -0.8f, 0.8f + juce::MathConstants<float>::pi, 0, true);
    p.addCentredArc(10, 14, 4, 4, -0.8f, 0.8f + juce::MathConstants<float>::pi, 0, false);
    // Stand
    p.addLineSegment({10.0f, 14.0f, 15.0f, 20.0f}, 1.5f);
    p.addLineSegment({8.0f, 20.0f, 17.0f, 20.0f}, 1.5f);
    // Signal waves
    p.addCentredArc(17, 8, 3, 3, -1.0f, 1.0f, 0, true);
    p.addCentredArc(17, 8, 5, 5, -1.0f, 1.0f, 0, true);
    return p;
}

static inline juce::Path server() {
    juce::Path p;
    p.addRoundedRectangle(4, 3, 16, 6, 2);
    p.addRectangle(6, 5, 2, 2);
    p.addRectangle(10, 5, 2, 2);
    p.addRectangle(14, 4.5f, 4, 1);
    p.addRectangle(14, 6.5f, 4, 1);
    p.addRoundedRectangle(4, 11, 16, 6, 2);
    p.addRectangle(6, 13, 2, 2);
    p.addRectangle(10, 13, 2, 2);
    p.addRectangle(14, 12.5f, 4, 1);
    p.addRectangle(14, 14.5f, 4, 1);
    p.addRectangle(6, 19, 2, 2);
    p.addRectangle(16, 19, 2, 2);
    return p;
}

static inline juce::Path music() {
    juce::Path p;
    p.addRoundedRectangle(8, 4, 10, 2, 1);
    p.addRectangle(8, 4, 2, 12);
    p.startNewSubPath(10, 4);
    p.lineTo(16, 6);
    p.lineTo(16, 8);
    p.lineTo(10, 6);
    p.closeSubPath();
    p.addRectangle(16, 6, 2, 10);
    p.startNewSubPath(18, 6);
    p.lineTo(22, 8);
    p.lineTo(22, 10);
    p.lineTo(18, 8);
    p.closeSubPath();
    p.addEllipse(5, 14, 5, 4);
    p.addEllipse(13, 14, 5, 4);
    return p;
}

static inline juce::Path audioFx() {
    juce::Path p;
    p.addRoundedRectangle(3, 5, 18, 2, 1);
    p.addRoundedRectangle(3, 11, 18, 2, 1);
    p.addRoundedRectangle(3, 17, 18, 2, 1);
    p.addEllipse(7, 3, 4, 6);
    p.addEllipse(14, 9, 4, 6);
    p.addEllipse(9, 15, 4, 6);
    return p;
}

static inline juce::Path mixer() {
    juce::Path p;
    p.addRoundedRectangle(5, 3, 2, 18, 1);
    p.addRoundedRectangle(11, 3, 2, 18, 1);
    p.addRoundedRectangle(17, 3, 2, 18, 1);
    p.addRoundedRectangle(3, 8, 6, 4, 1.5f);
    p.addRoundedRectangle(9, 12, 6, 4, 1.5f);
    p.addRoundedRectangle(15, 6, 6, 4, 1.5f);
    return p;
}

static inline juce::Path folder() {
    juce::Path p;
    p.startNewSubPath(2, 6);
    p.lineTo(2, 5);
    p.lineTo(9, 5);
    p.lineTo(10, 7);
    p.lineTo(22, 7);
    p.lineTo(22, 19);
    p.lineTo(2, 19);
    p.closeSubPath();
    return p;
}

static inline juce::Path play() {
    juce::Path p;
    p.addTriangle(6, 4, 20, 12, 6, 20);
    return p;
}

static inline juce::Path stop() {
    juce::Path p;
    p.addRoundedRectangle(5, 5, 14, 14, 2);
    return p;
}

static inline juce::Path connectIcon() {
    juce::Path p;
    p.addCentredArc(8, 12, 4, 4, 0.5f, juce::MathConstants<float>::pi - 0.5f, 0, true);
    p.addCentredArc(16, 12, 4, 4, -0.5f + juce::MathConstants<float>::pi,
                    juce::MathConstants<float>::pi + 0.5f, 0, true);
    p.addLineSegment({10.5f, 9.5f, 13.5f, 14.5f}, 2.0f);
    p.addLineSegment({10.5f, 14.5f, 13.5f, 9.5f}, 2.0f);
    return p;
}

static inline juce::Path disconnectIcon() {
    juce::Path p;
    p.addCentredArc(8, 12, 4, 4, 0.5f, juce::MathConstants<float>::pi - 0.5f, 0, true);
    p.addCentredArc(16, 12, 4, 4, -0.5f + juce::MathConstants<float>::pi,
                    juce::MathConstants<float>::pi + 0.5f, 0, true);
    p.addLineSegment({11, 10, 13, 14}, 1.5f);
    p.addLineSegment({13, 10, 11, 14}, 1.5f);
    return p;
}

static inline juce::Path microphone() {
    juce::Path p;
    p.addRoundedRectangle(9, 3, 6, 10, 3);
    p.addCentredArc(12, 13, 6, 6, 0, juce::MathConstants<float>::pi, 0, true);
    p.addLineSegment({12, 19, 12, 22}, 1.5f);
    p.addLineSegment({8, 22, 16, 22}, 1.5f);
    return p;
}

// Draw any icon centered in a rectangle
static inline void drawIcon(juce::Graphics& g, const juce::Path& iconPath,
                            juce::Rectangle<float> area, juce::Colour colour,
                            float padding = 4.0f) {
    auto iconArea = area.reduced(padding);
    auto bounds = iconPath.getBoundsTransformed(juce::AffineTransform());
    if (bounds.isEmpty()) return;

    float scaleX = iconArea.getWidth() / bounds.getWidth();
    float scaleY = iconArea.getHeight() / bounds.getHeight();
    float scale = std::min(scaleX, scaleY);

    float offsetX = iconArea.getX() + (iconArea.getWidth() - bounds.getWidth() * scale) * 0.5f;
    float offsetY = iconArea.getY() + (iconArea.getHeight() - bounds.getHeight() * scale) * 0.5f;

    juce::Path transformed = iconPath;
    transformed.applyTransform(
        juce::AffineTransform::translation(-bounds.getX(), -bounds.getY())
            .scaled(scale, scale)
            .translated(offsetX, offsetY));

    g.setColour(colour);
    g.fillPath(transformed);
}

} // namespace Icons
} // namespace ore
