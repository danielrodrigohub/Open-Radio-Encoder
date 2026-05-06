// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — VU Meter Component Implementation
// Renders vertical stereo VU meters at 60fps with smooth ballistics
// ═══════════════════════════════════════════════════════════════════════
#include "VUMeterComponent.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

VUMeterComponent::VUMeterComponent() {
    // Scale marks from -54dB to 0dB
    scaleMarks_ = {
        { 0.0f,   "0"   },
        { -6.0f,  "-6"  },
        { -12.0f, "-12" },
        { -18.0f, "-18" },
        { -24.0f, "-24" },
        { -30.0f, "-30" },
        { -36.0f, "-36" },
        { -42.0f, "-42" },
        { -48.0f, "-48" },
        { -54.0f, "-54" },
    };

    // Start 60fps timer
    startTimerHz(60);
}

void VUMeterComponent::setLevels(float peakDbL, float peakDbR,
                                  float /*rmsDbL*/, float /*rmsDbR*/) {
    targetL_.store(peakDbL, std::memory_order_relaxed);
    targetR_.store(peakDbR, std::memory_order_relaxed);
}

void VUMeterComponent::timerCallback() {
    // ── Ballistics: smooth the display values ──
    float tL = targetL_.load(std::memory_order_relaxed);
    float tR = targetR_.load(std::memory_order_relaxed);

    // Attack (fast) / Release (slow) envelope follower
    if (tL > displayL_) {
        displayL_ = displayL_ + (tL - displayL_) * kAttackCoeff;
    } else {
        displayL_ = displayL_ * kReleaseCoeff + tL * (1.0f - kReleaseCoeff);
    }

    if (tR > displayR_) {
        displayR_ = displayR_ + (tR - displayR_) * kAttackCoeff;
    } else {
        displayR_ = displayR_ * kReleaseCoeff + tR * (1.0f - kReleaseCoeff);
    }

    // ── Peak Hold ──
    if (displayL_ > peakHoldL_) {
        peakHoldL_ = displayL_;
        peakHoldCounterL_ = kPeakHoldFrames;
    } else if (peakHoldCounterL_ > 0) {
        peakHoldCounterL_--;
    } else {
        peakHoldL_ -= 0.5f; // Decay at 0.5 dB/frame (~30 dB/sec)
    }

    if (displayR_ > peakHoldR_) {
        peakHoldR_ = displayR_;
        peakHoldCounterR_ = kPeakHoldFrames;
    } else if (peakHoldCounterR_ > 0) {
        peakHoldCounterR_--;
    } else {
        peakHoldR_ -= 0.5f;
    }

    repaint();
}

float VUMeterComponent::dbToPosition(float dB) const {
    // Map -54..0 dB to 0..1 (bottom to top)
    float clamped = juce::jlimit(kMinDb, kMaxDb, dB);
    return (clamped - kMinDb) / (kMaxDb - kMinDb);
}

juce::Colour VUMeterComponent::getColorForPosition(float pos) const {
    // pos: 0 = bottom (-54dB), 1 = top (0dB)
    float dB = kMinDb + pos * (kMaxDb - kMinDb);

    if (dB > -6.0f) {
        // Red zone: -6 to 0 dB
        return juce::Colour(LookAndFeel_OpenRadio::kVURed);
    } else if (dB > -12.0f) {
        // Yellow zone: -12 to -6 dB
        return juce::Colour(LookAndFeel_OpenRadio::kVUYellow);
    } else {
        // Green zone: -54 to -12 dB
        return juce::Colour(LookAndFeel_OpenRadio::kVUGreen);
    }
}

void VUMeterComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();

    // ── Background ──
    g.setColour(juce::Colour(0xFF1A1A28));
    g.fillRect(bounds);

    // Layout:
    //   [Scale Labels] [Left Bar] [gap] [Right Bar] [dB label]
    auto scaleWidth = 30;
    auto barWidth = 14;
    auto gap = 3;
    auto topPad = 20;
    auto bottomPad = 10;

    auto meterHeight = bounds.getHeight() - topPad - bottomPad;
    auto leftBarX = scaleWidth;
    auto rightBarX = leftBarX + barWidth + gap;

    // ── "dB" label at top ──
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    g.setFont(juce::Font(11.0f));
    g.drawText("dB", bounds.removeFromTop(topPad), juce::Justification::centred);

    // ── Scale Labels ──
    g.setFont(juce::Font(10.0f));
    for (const auto& mark : scaleMarks_) {
        float pos = dbToPosition(mark.dB);
        int y = topPad + static_cast<int>((1.0f - pos) * meterHeight);
        g.setColour(juce::Colour(LookAndFeel_OpenRadio::kTextDim));
        g.drawText(mark.label, 0, y - 6, scaleWidth - 4, 12,
                   juce::Justification::centredRight);

        // Tick mark
        g.setColour(juce::Colour(0xFF404050));
        g.drawHorizontalLine(y, (float)scaleWidth - 2, (float)(rightBarX + barWidth + 2));
    }

    // ── Draw Meter Bars ──
    auto drawBar = [&](int x, float levelDb, float peakDb) {
        float levelPos = dbToPosition(levelDb);
        float peakPos = dbToPosition(peakDb);

        int levelY = topPad + static_cast<int>((1.0f - levelPos) * meterHeight);
        int barBottom = topPad + meterHeight;

        // Dark background for the bar channel
        g.setColour(juce::Colour(0xFF0A0A14));
        g.fillRect(x, topPad, barWidth, meterHeight);

        // Filled gradient from bottom to level
        if (levelDb > kMinDb) {
            // Draw segmented bar with color zones
            int numSegments = barBottom - levelY;
            for (int i = 0; i < numSegments; i++) {
                int py = barBottom - i;
                float segPos = static_cast<float>(i) / meterHeight;
                g.setColour(getColorForPosition(segPos));
                g.fillRect(x + 1, py, barWidth - 2, 1);
            }
        }

        // Peak hold indicator (white line)
        if (peakDb > kMinDb + 1.0f) {
            int peakY = topPad + static_cast<int>((1.0f - peakPos) * meterHeight);
            g.setColour(juce::Colours::white.withAlpha(0.9f));
            g.fillRect(x, peakY, barWidth, 2);
        }
    };

    drawBar(leftBarX, displayL_, peakHoldL_);
    drawBar(rightBarX, displayR_, peakHoldR_);

    // ── Border ──
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
    g.drawRect(bounds.withTrimmedTop(-topPad), 1);
}

} // namespace ore
