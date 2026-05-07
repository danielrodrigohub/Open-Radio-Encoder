#pragma once
#include <JuceHeader.h>

namespace ore {

class RecordingPanel : public juce::Component {
public:
    RecordingPanel();
    void paint(juce::Graphics& g) override;
    void resized() override;

    void setRecordingActive(bool active);
    void setRecordingSize(const juce::String& size);
    void setRecordingElapsed(const juce::String& elapsed);
    void setRecordingPath(const juce::String& path);

    std::function<void(const juce::String& path, const juce::String& codec, int quality)> onStartRecording;
    std::function<void()> onStopRecording;

    const juce::String& currentPath() const { return currentPath_; }
    const juce::String& currentCodec() const { return currentCodec_; }
    int currentQuality() const { return qualityBox_.getText().getIntValue(); }

private:
    void browseForPath();
    void updateQualityOptions();

    juce::Label headerLabel_{"", "Recording"};

    juce::ComboBox codecBox_;
    juce::Label codecLabel_{"", "Codec"};

    juce::ComboBox qualityBox_;
    juce::Label qualityLabel_{"", "Quality"};

    juce::TextEditor pathEditor_;
    juce::TextButton browseBtn_{"..."};

    juce::TextButton startBtn_{"Start recording"};
    juce::TextButton stopBtn_{"Stop recording"};

    juce::Label dataSizeLabel_;
    juce::Label dataSizeLabelHeader_{"", "Recording Data"};
    juce::Label elapsedLabel_;
    juce::Label elapsedLabelHeader_{"", "Elapsed"};
    juce::Label pathLabel_;

    juce::String currentPath_;
    juce::String currentCodec_ = "wav";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecordingPanel)
};

} // namespace ore
