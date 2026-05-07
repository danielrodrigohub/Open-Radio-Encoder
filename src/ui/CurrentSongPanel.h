#pragma once
#include <JuceHeader.h>
#include <atomic>

namespace ore {

class CurrentSongPanel : public juce::Component {
public:
    CurrentSongPanel();
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(const juce::String&)> onUpdateSong;

    void pollFileIfNeeded();

private:
    void browseForFile();

    juce::Label headerLabel_{"", "Current Song"};
    juce::TextEditor songEditor_;
    juce::TextButton updateBtn_{"Update"};

    // TXT file mode
    juce::ToggleButton txtModeToggle_{"TXT File"};
    juce::TextEditor txtPathEditor_;
    juce::TextButton txtBrowseBtn_{"..."};
    juce::Label txtStatusLabel_{"", ""};

    juce::Time lastModTime_;
    juce::String lastTxtContent_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CurrentSongPanel)
};

} // namespace ore
