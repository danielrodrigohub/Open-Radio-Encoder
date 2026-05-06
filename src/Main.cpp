// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — JUCE Application Entry Point
// ═══════════════════════════════════════════════════════════════════════
#include <JuceHeader.h>
#include "ui/MainWindow.h"
#include "ui/LookAndFeel_OpenRadio.h"

class OpenRadioEncoderApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override    { return "Open Radio Encoder"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override           { return false; }

    void initialise(const juce::String& /*commandLine*/) override {
        lookAndFeel_ = std::make_unique<ore::LookAndFeel_OpenRadio>();
        juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel_.get());

        mainWindow_ = std::make_unique<ore::MainWindow>(
            getApplicationName() + " " + getApplicationVersion());
    }

    void shutdown() override {
        mainWindow_ = nullptr;
        lookAndFeel_ = nullptr;
    }

    void systemRequestedQuit() override {
        quit();
    }

private:
    std::unique_ptr<ore::MainWindow> mainWindow_;
    std::unique_ptr<ore::LookAndFeel_OpenRadio> lookAndFeel_;
};

START_JUCE_APPLICATION(OpenRadioEncoderApplication)
