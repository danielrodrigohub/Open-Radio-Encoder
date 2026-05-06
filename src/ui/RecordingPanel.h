// Recording Panel
#pragma once
#include <JuceHeader.h>

namespace ore {

class RecordingPanel : public juce::Component {
public:
    RecordingPanel();
    void paint(juce::Graphics& g) override;
    void resized() override;

    void setRecordingSize(const juce::String& size);
    void setRecordingPath(const juce::String& path);

    std::function<void()> onStartRecording;
    std::function<void()> onStopRecording;

private:
    juce::Label headerLabel_{"", "Recording"};
    juce::TextButton startBtn_{"Start recording"};
    juce::TextButton stopBtn_{"Stop recording"};
    juce::TextButton browseBtn_;  // folder icon
    juce::Label dataSizeLabel_;
    juce::Label dataSizeLabelHeader_{"", "Recording Data"};
    juce::Label pathLabel_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingPanel)
};

} // namespace ore
