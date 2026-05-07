#include "ConfigManager.h"
#include <cstdio>

namespace ore {

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

juce::File ConfigManager::configFile() const {
    auto dir = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
                   .getChildFile("OpenRadioEncoder");
    dir.createDirectory();
    return dir.getChildFile("stations.json");
}

juce::var ConfigManager::stationToVar(const StationConfig& s) const {
    juce::DynamicObject* obj = new juce::DynamicObject();
    obj->setProperty("id", s.id);
    obj->setProperty("name", juce::String(s.name));
    obj->setProperty("enabled", s.enabled);
    obj->setProperty("serverType", s.serverType == ServerType::Icecast ? "icecast" : "shoutcast");
    obj->setProperty("address", juce::String(s.address));
    obj->setProperty("port", s.port);
    obj->setProperty("mountPoint", juce::String(s.mountPoint));
    obj->setProperty("username", juce::String(s.username));
    obj->setProperty("password", juce::String(s.password));
    obj->setProperty("useTLS", s.useTLS);
    obj->setProperty("icyName", juce::String(s.icyName));
    obj->setProperty("icyGenre", juce::String(s.icyGenre));
    obj->setProperty("icyUrl", juce::String(s.icyUrl));
    obj->setProperty("icyPublic", s.icyPublic);

    // Encoder config
    obj->setProperty("codec", static_cast<int>(s.encoder.codec));
    obj->setProperty("samplerate", s.encoder.samplerate);
    obj->setProperty("channels", s.encoder.channels);
    obj->setProperty("bitrate", s.encoder.bitrate);
    obj->setProperty("quality", s.encoder.quality);

    return juce::var(obj);
}

StationConfig ConfigManager::varToStation(const juce::var& v) const {
    StationConfig s;
    s.id = v.getProperty("id", 0);
    s.name = v.getProperty("name", "Station").toString().toStdString();
    s.enabled = v.getProperty("enabled", true);
    juce::String type = v.getProperty("serverType", "icecast").toString();
    s.serverType = (type == "shoutcast") ? ServerType::Shoutcast : ServerType::Icecast;
    s.address = v.getProperty("address", "localhost").toString().toStdString();
    s.port = v.getProperty("port", 8000);
    s.mountPoint = v.getProperty("mountPoint", "/stream").toString().toStdString();
    s.username = v.getProperty("username", "source").toString().toStdString();
    s.password = v.getProperty("password", "").toString().toStdString();
    s.useTLS = v.getProperty("useTLS", false);
    s.icyName = v.getProperty("icyName", "").toString().toStdString();
    s.icyGenre = v.getProperty("icyGenre", "").toString().toStdString();
    s.icyUrl = v.getProperty("icyUrl", "").toString().toStdString();
    s.icyPublic = v.getProperty("icyPublic", false);

    s.encoder.codec = static_cast<CodecType>(static_cast<int>(v.getProperty("codec", 0)));
    s.encoder.samplerate = v.getProperty("samplerate", 44100);
    s.encoder.channels = v.getProperty("channels", 2);
    s.encoder.bitrate = v.getProperty("bitrate", 128);
    s.encoder.quality = v.getProperty("quality", 5);

    return s;
}

void ConfigManager::save(const std::vector<StationConfig>& stations) {
    juce::Array<juce::var> arr;
    for (const auto& s : stations) {
        arr.add(stationToVar(s));
    }

    juce::var json(arr);
    juce::String text = juce::JSON::toString(json);

    auto file = configFile();
    if (file.replaceWithText(text)) {
        fprintf(stdout, "[Config] Saved %zu stations to %s\n",
                stations.size(), file.getFullPathName().toRawUTF8());
    } else {
        fprintf(stderr, "[Config] Failed to save config\n");
    }
}

std::vector<StationConfig> ConfigManager::load() {
    std::vector<StationConfig> stations;

    auto file = configFile();
    if (!file.existsAsFile()) {
        fprintf(stdout, "[Config] No config file found, using defaults\n");
        return stations;
    }

    juce::String text = file.loadFileAsString();
    juce::var json = juce::JSON::parse(text);

    if (auto* arr = json.getArray()) {
        for (const auto& v : *arr) {
            stations.push_back(varToStation(v));
        }
        fprintf(stdout, "[Config] Loaded %zu stations from %s\n",
                stations.size(), file.getFullPathName().toRawUTF8());
    }

    return stations;
}

} // namespace ore
