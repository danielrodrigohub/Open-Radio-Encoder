#pragma once
#include <JuceHeader.h>
#include "engine/BroadcastDistributor.h"

namespace ore {

class StationEditor : public juce::Component {
public:
    StationEditor();
    void loadFrom(const StationConfig& config);
    StationConfig toConfig() const;
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::TextEditor nameEdit_, addressEdit_, portEdit_, mountEdit_, userEdit_, passEdit_;
    juce::ComboBox serverTypeBox_, codecBox_, samplerateBox_, channelsBox_, reconnectIntervalBox_, bitrateBox_;
    juce::ToggleButton tlsToggle_{"TLS"};
};

class ManagerDialog : public juce::Component, public juce::ListBoxModel {
public:
    explicit ManagerDialog(BroadcastDistributor* dist);
    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override { return static_cast<int>(stationIds_.size()); }
    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override { selectStation(row); }
    void selectedRowsChanged(int) override {}

    void refreshFromDistributor();
    void selectStationById(int stationId);

    std::function<void()> onStationsChanged;
    std::function<void(const StationConfig&)> onStationAdded;
    std::function<void(int stationId)> onConnectStation;
    std::function<void(int stationId)> onDisconnectStation;
    std::function<void(int)> onNavigateToStreaming;

private:
    void selectStation(int row);
    void applyCurrent();
    void deleteCurrent();
    void addStation();
    std::string formatStationStatus(int stationId) const;

    BroadcastDistributor* dist_;
    StationEditor editor_;
    juce::ListBox stationList_{"Stations", this};
    juce::TextButton addBtn_{"Add"}, applyBtn_{"Apply"}, deleteBtn_{"Delete"};
    juce::TextButton connectBtn_{"Connect"}, disconnectBtn_{"Disconnect"};
    juce::Label header_{"", "Station Manager"};

    std::vector<int> stationIds_;
    int currentEdit_ = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ManagerDialog)
};

} // namespace ore
