// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Main Window Implementation
// Layout replicates the BUTTM 1.0.0 - Pro reference image
// ═══════════════════════════════════════════════════════════════════════
#include "MainWindow.h"
#include "LookAndFeel_OpenRadio.h"

namespace ore {

// ─────────────────────────────────────────────
// MainContentComponent
// ─────────────────────────────────────────────
MainContentComponent::MainContentComponent() {
    addAndMakeVisible(toolbar_);
    addAndMakeVisible(streamingPanel_);
    addAndMakeVisible(currentSongPanel_);
    addAndMakeVisible(recordingPanel_);
    addAndMakeVisible(vuMeter_);

    setSize(980, 900);
}

void MainContentComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(LookAndFeel_OpenRadio::kBackground));
}

void MainContentComponent::resized() {
    auto bounds = getLocalBounds();

    // ── Toolbar (top, full width) ──
    toolbar_.setBounds(bounds.removeFromTop(70));

    // ── Right side: VU Meter (fixed width 80px) ──
    auto vuArea = bounds.removeFromRight(80);
    vuMeter_.setBounds(vuArea.reduced(5));

    // ── Left side: Streaming + Song + Recording panels ──
    auto leftArea = bounds.reduced(10, 5);

    // "Streaming" group label
    auto streamingHeight = leftArea.getHeight() * 0.55;
    streamingPanel_.setBounds(leftArea.removeFromTop(static_cast<int>(streamingHeight)));

    leftArea.removeFromTop(5); // spacing

    // Current Song panel
    currentSongPanel_.setBounds(leftArea.removeFromTop(80));

    leftArea.removeFromTop(5); // spacing

    // Recording panel (fills remaining)
    recordingPanel_.setBounds(leftArea);
}

// ─────────────────────────────────────────────
// MainWindow (DocumentWindow wrapper)
// ─────────────────────────────────────────────
MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name, juce::Colour(LookAndFeel_OpenRadio::kBackground),
                     DocumentWindow::allButtons) {
    setUsingNativeTitleBar(true);
    setContentOwned(new MainContentComponent(), true);

    setResizable(true, true);
    setResizeLimits(800, 700, 1600, 1200);
    centreWithSize(getWidth(), getHeight());

    setVisible(true);
}

void MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

} // namespace ore
