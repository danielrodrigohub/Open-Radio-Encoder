// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — JUCE Application Entry Point
// ═══════════════════════════════════════════════════════════════════════
#include <JuceHeader.h>
#include "ui/MainWindow.h"
#include "ui/LookAndFeel_OpenRadio.h"

static void printHelp() {
    std::string help = R"(Open Radio Encoder v1.0.0
Multi-station internet radio streaming encoder.

Usage:
  OpenRadioEncoder [FLAGS]

Flags:
  -h, --help              Show this help message and exit
  -v, --version           Show version information and exit
  -c, --config <path>     Path to stations.json config file
                          (default: <user_app_data>/OpenRadioEncoder/stations.json)
  --headless              Run without GUI (for server/headless use)
  --verbose               Enable verbose logging

Examples:
  OpenRadioEncoder
  OpenRadioEncoder --config /etc/radio/stations.json
  OpenRadioEncoder --headless --verbose

Made in Chile with Love!
)";
    fprintf(stdout, "%s", help.c_str());
}

static void printVersion() {
    fprintf(stdout, "Open Radio Encoder v1.0.0\n");
}

class OpenRadioEncoderApplication : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override    { return "Open Radio Encoder"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override           { return false; }

    void initialise(const juce::String& commandLine) override {
        juce::StringArray argTokens;
        argTokens.addTokens(commandLine, true);
        argTokens.removeEmptyStrings();
        juce::ArgumentList args(getApplicationName(), argTokens);

        if (args.containsOption("--help") || args.containsOption("-h")) {
            printHelp();
            systemRequestedQuit();
            return;
        }

        if (args.containsOption("--version") || args.containsOption("-v")) {
            printVersion();
            systemRequestedQuit();
            return;
        }

        if (args.containsOption("--verbose")) {
            fprintf(stdout, "[Main] Verbose logging enabled\n");
        }

        bool headless = args.containsOption("--headless");
        juce::String configPath;
        if (args.containsOption("--config") || args.containsOption("-c")) {
            configPath = args.getValueForOption("--config");
            if (configPath.isEmpty()) {
                configPath = args.getValueForOption("-c");
            }
            fprintf(stdout, "[Main] Using config: %s\n", configPath.toRawUTF8());
        }

        if (!headless) {
            lookAndFeel_ = std::make_unique<ore::LookAndFeel_OpenRadio>();
            juce::LookAndFeel::setDefaultLookAndFeel(lookAndFeel_.get());

            mainWindow_ = std::make_unique<ore::MainWindow>(
                getApplicationName() + " " + getApplicationVersion());
        } else {
            fprintf(stdout, "[Main] Headless mode — no GUI\n");
            fprintf(stderr, "[Main] Headless mode not yet fully implemented. Use GUI mode.\n");
            systemRequestedQuit();
        }
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
