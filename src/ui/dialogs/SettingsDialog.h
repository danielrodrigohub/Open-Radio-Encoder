#pragma once
#include <JuceHeader.h>
#include "engine/AudioPipeline.h"

namespace ore {

class SettingsDialog : public juce::Component {
public:
    SettingsDialog(AudioPipeline* pipeline);
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onSettingsChanged;

private:
    void applySettings();

    AudioPipeline* pipeline_;

    juce::Label header_{"", "Settings"};
    juce::Label audioGroup_{"", "Audio Input"};
    juce::ComboBox deviceCombo_;
    juce::Label deviceLabel_{"", "Device"};
    juce::ComboBox sampleRateCombo_;
    juce::Label sampleRateLabel_{"", "Sample Rate"};
    juce::ComboBox bufferSizeCombo_;
    juce::Label bufferSizeLabel_{"", "Buffer Size"};

    juce::Label recordingGroup_{"", "Recording"};
    juce::TextEditor recordingPathEditor_;
    juce::Label recordingPathLabel_{"", "Output Folder"};
    juce::TextButton browseBtn_{"Browse"};

    juce::TextButton applyBtn_{"Apply Settings"};
    juce::TextButton cancelBtn_{"Cancel"};
    juce::Label statusLabel_{"", ""};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsDialog)
};

} // namespace ore
