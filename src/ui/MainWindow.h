#pragma once
#include <JuceHeader.h>
#include "ToolbarComponent.h"
#include "StreamingPanel.h"
#include "CurrentSongPanel.h"
#include "RecordingPanel.h"
#include "VUMeterComponent.h"
#include "ConsoleLogComponent.h"
#include "engine/AudioPipeline.h"
#include "engine/BroadcastDistributor.h"
#include "engine/StationConnection.h"
#include "dialogs/SettingsDialog.h"
#include "dialogs/AudioFxDialog.h"
#include "dialogs/MixerDialog.h"
#include "dialogs/SchedulerDialog.h"
#include "dialogs/ManagerDialogs.h"

namespace ore {

class MainContentComponent : public juce::Component,
                              public juce::Timer {
public:
    MainContentComponent();
    ~MainContentComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    void setupEngine();
    void wireUI();
    void addStationBlock(const StationConfig& sc);
    void rebuildStreamingBlocks();
    void saveAllStations(BroadcastDistributor* dist);
    void showTab(int index);

    ToolbarComponent toolbar_;

    StreamingPanel streamingPanel_;
    CurrentSongPanel currentSongPanel_;
    RecordingPanel recordingPanel_;
    VUMeterComponent vuMeter_;

    std::unique_ptr<SettingsDialog> settingsView_;
    std::unique_ptr<SchedulerDialog> schedulerView_;
    std::unique_ptr<ManagerDialog> stationMgrView_;
    std::unique_ptr<ConsoleLogComponent> consoleLogView_;
    std::unique_ptr<AudioFxDialog> audioFxView_;
    std::unique_ptr<MixerDialog> mixerView_;

    int currentTab_ = 0;

    std::unique_ptr<AudioPipeline> pipeline_;
    bool engineRunning_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainContentComponent)
};

class MainWindow : public juce::DocumentWindow {
public:
    explicit MainWindow(const juce::String& name);
    void closeButtonPressed() override;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

} // namespace ore
