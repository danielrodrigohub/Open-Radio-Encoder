#pragma once
#include <JuceHeader.h>
#include <functional>
#include <vector>

namespace ore {

struct ScheduledTask {
    int hour = 0;
    int minute = 0;
    int action = 0;
    juce::String description;
};

class SchedulerDialog : public juce::Component, public juce::ListBoxModel, public juce::Timer {
public:
    SchedulerDialog();
    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    int getNumRows() override { return static_cast<int>(tasks_.size()); }
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;

    std::function<void()> onConnectAll;
    std::function<void()> onDisconnectAll;
    std::function<void()> onStartRecording;
    std::function<void()> onStopRecording;

private:
    void addTask();
    void checkTasks();

    juce::Label header_{"", "Scheduler"};
    juce::Label timeLabel_{"", "--:--:--"};
    juce::Label nextLabel_{"", "No tasks scheduled"};
    juce::TextButton addBtn_{"Add Task"};
    juce::TextButton removeBtn_{"Remove Selected"};
    juce::TextButton clearBtn_{"Clear All"};

    std::vector<ScheduledTask> tasks_;
    juce::ListBox taskList_{"Tasks", this};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SchedulerDialog)
};

} // namespace ore
