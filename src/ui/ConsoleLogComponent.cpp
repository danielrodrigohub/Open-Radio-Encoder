#include "ConsoleLogComponent.h"
#include "LookAndFeel_OpenRadio.h"
#include <deque>

namespace ore {

ConsoleLogComponent* ConsoleLogComponent::instance = nullptr;

ConsoleLogComponent::ConsoleLogComponent() {
    instance = this;
    juce::Logger::setCurrentLogger(this);

    header_.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    addAndMakeVisible(header_);

    logDisplay_.setMultiLine(true);
    logDisplay_.setReadOnly(true);
    logDisplay_.setScrollbarsShown(true);
    logDisplay_.setCaretVisible(false);
    logDisplay_.setFont(juce::Font(juce::FontOptions("Courier New", 11.0f, juce::Font::plain)));
    logDisplay_.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF0E0E18));
    logDisplay_.setColour(juce::TextEditor::textColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    addAndMakeVisible(logDisplay_);

    clearBtn_.onClick = [this]() {
        juce::MessageManagerLock lock;
        logDisplay_.clear();
    };
    addAndMakeVisible(clearBtn_);

    copyBtn_.onClick = [this]() {
        juce::SystemClipboard::copyTextToClipboard(logDisplay_.getText());
    };
    addAndMakeVisible(copyBtn_);

    juce::Logger::writeToLog("[Console] Log started");
    startTimerHz(10);
}

ConsoleLogComponent::~ConsoleLogComponent() {
    juce::Logger::setCurrentLogger(nullptr);
    if (instance == this) instance = nullptr;
}

void ConsoleLogComponent::logMessage(const juce::String& message) {
    // Generate ISO timestamp: [2026-05-06*18:30:00]
    auto now = juce::Time::getCurrentTime();
    auto timestamp = now.formatted("%Y-%m-%d*%H:%M:%S");
    juce::String formattedMsg = "[" + timestamp + "] " + message;

    if (!juce::MessageManager::getInstance()->isThisTheMessageThread()) {
        std::lock_guard<std::mutex> lock(logMutex_);
        pendingMessages_.push_back(formattedMsg);
    } else {
        logDisplay_.insertTextAtCaret(formattedMsg + "\n");
        while (logDisplay_.getTotalNumChars() > kMaxLines * 80) {
            auto text = logDisplay_.getText();
            auto idx = text.indexOf(0, "\n");
            if (idx < 0) break;
            logDisplay_.setText(text.substring(idx + 1));
        }
    }
}

void ConsoleLogComponent::timerCallback() {
    std::lock_guard<std::mutex> lock(logMutex_);
    if (pendingMessages_.empty()) return;

    int count = std::min(50, static_cast<int>(pendingMessages_.size()));
    for (int i = 0; i < count; i++) {
        logDisplay_.insertTextAtCaret(pendingMessages_.front() + "\n");
        pendingMessages_.pop_front();
    }
    pendingMessages_.clear();

    logDisplay_.moveCaretToEnd();
}

void ConsoleLogComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(LookAndFeel_OpenRadio::kBackground));
}

void ConsoleLogComponent::resized() {
    auto b = getLocalBounds().reduced(12);
    header_.setBounds(b.removeFromTop(28));
    b.removeFromTop(6);

    auto btnRow = b.removeFromTop(30);
    copyBtn_.setBounds(btnRow.removeFromRight(100));
    btnRow.removeFromRight(8);
    clearBtn_.setBounds(btnRow.removeFromRight(80));

    b.removeFromTop(6);
    logDisplay_.setBounds(b);
}

} // namespace ore
