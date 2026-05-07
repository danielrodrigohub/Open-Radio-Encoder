#pragma once
#include <JuceHeader.h>
#include "engine/BroadcastDistributor.h"
#include <vector>

namespace ore {

class ConfigManager {
public:
    static ConfigManager& getInstance();

    void save(const std::vector<StationConfig>& stations);
    std::vector<StationConfig> load();
    juce::File configFile() const;

private:
    ConfigManager() = default;

    juce::var stationToVar(const StationConfig& s) const;
    StationConfig varToStation(const juce::var& v) const;
};

} // namespace ore
