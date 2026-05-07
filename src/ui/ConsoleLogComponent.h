#pragma once
#include <JuceHeader.h>
#include <deque>
#include <mutex>

namespace ore {

class ConsoleLogComponent : public juce::Component, public juce::Logger, public juce::Timer {
public:
    ConsoleLogComponent();
    ~ConsoleLogComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    static ConsoleLogComponent* instance;

private:
    void logMessage(const juce::String& message) override;

    juce::Label header_{"", "Console Log"};
    juce::TextEditor logDisplay_;
    juce::TextButton clearBtn_{"Clear"};
    juce::TextButton copyBtn_{"Copy All"};

    std::deque<juce::String> pendingMessages_;
    std::mutex logMutex_;

    static constexpr int kMaxLines = 1000;
};

} // namespace ore
