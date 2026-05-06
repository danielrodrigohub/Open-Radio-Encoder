// Current Song Panel
#pragma once
#include <JuceHeader.h>

namespace ore {

class CurrentSongPanel : public juce::Component {
public:
    CurrentSongPanel();
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(const juce::String&)> onUpdateSong;

private:
    juce::Label headerLabel_{"", "Current Song"};
    juce::TextEditor songEditor_;
    juce::TextButton updateBtn_{"Update"};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurrentSongPanel)
};

} // namespace ore
