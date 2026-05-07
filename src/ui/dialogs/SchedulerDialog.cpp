#include "SchedulerDialog.h"
#include "ui/LookAndFeel_OpenRadio.h"
#include <cstdio>

namespace ore {

static const char* kActions[] = {"Connect All", "Disconnect All", "Start Recording", "Stop Recording"};
static const char* kActionIcons[] = {">>", "[]", "o", "--"};
static const uint32_t kActionColors[] = {
    LookAndFeel_OpenRadio::kAccentGreen,
    LookAndFeel_OpenRadio::kAccentRed,
    LookAndFeel_OpenRadio::kAccentBlue,
    LookAndFeel_OpenRadio::kVUYellow
};

SchedulerDialog::SchedulerDialog() {
    header_.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    addAndMakeVisible(header_);

    timeLabel_.setFont(juce::Font(juce::FontOptions(36.0f, juce::Font::bold)));
    timeLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    timeLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timeLabel_);

    nextLabel_.setFont(juce::Font(juce::FontOptions(13.0f)));
    nextLabel_.setColour(juce::Label::textColourId, juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    nextLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(nextLabel_);

    addBtn_.setColour(juce::TextButton::buttonColourId, juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    addBtn_.onClick = [this]() { addTask(); };
    addAndMakeVisible(addBtn_);

    removeBtn_.onClick = [this]() {
        int row = taskList_.getSelectedRow();
        if (row >= 0 && row < static_cast<int>(tasks_.size())) {
            tasks_.erase(tasks_.begin() + row);
            taskList_.updateContent();
            taskList_.repaint();
        }
    };
    addAndMakeVisible(removeBtn_);

    clearBtn_.onClick = [this]() {
        tasks_.clear();
        taskList_.updateContent();
        taskList_.repaint();
        nextLabel_.setText("No tasks scheduled", juce::dontSendNotification);
    };
    addAndMakeVisible(clearBtn_);

    taskList_.setRowHeight(32);
    addAndMakeVisible(taskList_);

    setSize(500, 420);
    startTimerHz(2);
}

void SchedulerDialog::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(LookAndFeel_OpenRadio::kBackground));

    auto b = getLocalBounds().toFloat().reduced(15.0f);

    float clockAreaY = b.getY() + 30;
    float clockAreaH = 80;
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kSurface));
    g.fillRoundedRectangle(b.getX(), clockAreaY, b.getWidth(), clockAreaH, 6.0f);
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
    g.drawRoundedRectangle(b.getX(), clockAreaY, b.getWidth(), clockAreaH, 6.0f, 1.0f);
}

void SchedulerDialog::resized() {
    auto b = getLocalBounds().reduced(15);
    header_.setBounds(b.removeFromTop(28));

    auto clockArea = b.removeFromTop(80).reduced(8, 6);
    timeLabel_.setBounds(clockArea.removeFromTop(42));
    nextLabel_.setBounds(clockArea.removeFromTop(20));

    b.removeFromTop(8);

    auto btnRow = b.removeFromTop(32);
    addBtn_.setBounds(btnRow.removeFromLeft(100));
    btnRow.removeFromLeft(8);
    removeBtn_.setBounds(btnRow.removeFromLeft(130));
    btnRow.removeFromLeft(8);
    clearBtn_.setBounds(btnRow.removeFromLeft(80));

    b.removeFromTop(6);
    taskList_.setBounds(b);
}

void SchedulerDialog::timerCallback() {
    auto now = juce::Time::getCurrentTime();
    timeLabel_.setText(now.formatted("%H:%M:%S"), juce::dontSendNotification);
    checkTasks();
}

void SchedulerDialog::addTask() {
    class TaskForm : public juce::Component {
    public:
        juce::ComboBox hourBox, minBox, actionBox;
        juce::Label hourLbl{"", "Hour"}, minLbl{"", "Min"}, actionLbl{"", "Action"};
        TaskForm() {
            auto font = juce::Font(juce::FontOptions(11.0f));
            hourLbl.setFont(font); minLbl.setFont(font); actionLbl.setFont(font);
            addAndMakeVisible(hourLbl); addAndMakeVisible(minLbl); addAndMakeVisible(actionLbl);

            for (int h = 0; h < 24; h++) hourBox.addItem(juce::String(h).paddedLeft('0', 2), h+1);
            for (int m = 0; m < 60; m+=5) minBox.addItem(juce::String(m).paddedLeft('0', 2), m/5+1);
            for (int a = 0; a < 4; a++) actionBox.addItem(kActions[a], a+1);
            hourBox.setSelectedId(1); minBox.setSelectedId(1); actionBox.setSelectedId(1);
            addAndMakeVisible(hourBox); addAndMakeVisible(minBox); addAndMakeVisible(actionBox);
            setSize(380, 100);
        }
        void resized() override {
            auto b = getLocalBounds().reduced(4);
            int colW = b.getWidth() / 3;
            auto col1 = b.removeFromLeft(colW).reduced(4);
            hourLbl.setBounds(col1.removeFromTop(14));
            hourBox.setBounds(col1);
            auto col2 = b.removeFromLeft(colW).reduced(4);
            minLbl.setBounds(col2.removeFromTop(14));
            minBox.setBounds(col2);
            auto col3 = b.reduced(4);
            actionLbl.setBounds(col3.removeFromTop(14));
            actionBox.setBounds(col3);
        }
    };

    juce::AlertWindow win("Schedule a Task",
        "Configure the task execution time and action:",
        juce::AlertWindow::QuestionIcon);
    TaskForm form;
    win.addCustomComponent(&form);
    win.addButton("OK", 1);
    win.addButton("Cancel", 0);

    if (win.runModalLoop() != 0) {
        ScheduledTask t;
        t.hour = form.hourBox.getSelectedItemIndex();
        t.minute = (form.minBox.getSelectedItemIndex()) * 5;
        t.action = form.actionBox.getSelectedItemIndex();
        t.description = juce::String(kActions[t.action]);
        tasks_.push_back(t);
        std::sort(tasks_.begin(), tasks_.end(), [](const auto& a, const auto& b) {
            return (a.hour * 60 + a.minute) < (b.hour * 60 + b.minute);
        });
        taskList_.updateContent();
        taskList_.repaint();
        fprintf(stdout, "[Scheduler] Added task: %02d:%02d - %s\n",
                t.hour, t.minute, kActions[t.action]);
    }
}

void SchedulerDialog::checkTasks() {
    auto now = juce::Time::getCurrentTime();
    int currentSeconds = now.getHours() * 3600 + now.getMinutes() * 60 + now.getSeconds();

    ScheduledTask* nextTask = nullptr;
    int nextTaskSeconds = 86400;

    for (auto& t : tasks_) {
        int taskSeconds = (t.hour * 3600 + t.minute * 60) % 86400;
        if (currentSeconds >= taskSeconds && currentSeconds < taskSeconds + 2) {
            fprintf(stdout, "[Scheduler] Executing: %02d:%02d - %s\n",
                    t.hour, t.minute, kActions[t.action]);
            switch (t.action) {
                case 0: if (onConnectAll) onConnectAll(); break;
                case 1: if (onDisconnectAll) onDisconnectAll(); break;
                case 2: if (onStartRecording) onStartRecording(); break;
                case 3: if (onStopRecording) onStopRecording(); break;
            }
        }
        if (taskSeconds > currentSeconds && taskSeconds < nextTaskSeconds) {
            nextTaskSeconds = taskSeconds;
            nextTask = &t;
        }
    }

    if (nextTask) {
        nextLabel_.setText("Next: " + juce::String(nextTask->hour).paddedLeft('0', 2) +
                          ":" + juce::String(nextTask->minute).paddedLeft('0', 2) +
                          " - " + juce::String(kActions[nextTask->action]),
                          juce::dontSendNotification);
    } else if (!tasks_.empty()) {
        auto& first = tasks_.front();
        nextLabel_.setText("Next: " + juce::String(first.hour).paddedLeft('0', 2) +
                          ":" + juce::String(first.minute).paddedLeft('0', 2) +
                          " (tomorrow) - " + juce::String(kActions[first.action]),
                          juce::dontSendNotification);
    } else {
        nextLabel_.setText("No tasks scheduled", juce::dontSendNotification);
    }
}

void SchedulerDialog::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) {
    if (row < 0 || row >= static_cast<int>(tasks_.size())) return;

    g.setColour(selected ? juce::Colour(LookAndFeel_OpenRadio::kSurfaceHover)
                         : juce::Colour(LookAndFeel_OpenRadio::kSurface));
    g.fillRoundedRectangle(1.0f, 1.0f, static_cast<float>(w) - 2, static_cast<float>(h) - 2, 3.0f);

    auto& t = tasks_[static_cast<size_t>(row)];

    g.setFont(juce::Font(juce::FontOptions("Courier New", 14.0f, juce::Font::bold)));
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
    juce::String timeStr = juce::String(t.hour).paddedLeft('0', 2) + ":" +
                           juce::String(t.minute).paddedLeft('0', 2);
    g.drawText(timeStr, 10, 0, 60, h, juce::Justification::centredLeft, true);

    g.setFont(juce::Font(juce::FontOptions(12.0f)));
    g.setColour(juce::Colour(kActionColors[std::min(t.action, 3)]));
    juce::String prefix = juce::String(kActionIcons[std::min(t.action, 3)]) + " ";
    g.drawText(prefix + kActions[std::min(t.action, 3)],
                75, 0, w - 90, h, juce::Justification::centredLeft, true);
}

} // namespace ore
