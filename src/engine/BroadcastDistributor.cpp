// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Broadcast Distributor Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "BroadcastDistributor.h"
#include "StationConnection.h"

#include <algorithm>

namespace ore {

BroadcastDistributor::BroadcastDistributor() = default;

BroadcastDistributor::~BroadcastDistributor() {
    disconnectAll();
}

int BroadcastDistributor::addStation(const StationConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto station = std::make_unique<StationConnection>();
    auto cfg = config;

    if (cfg.id == 0) {
        cfg.id = nextId_++;
        // If the name is generic (starts with "Station"), update it to reflect the new ID
        if (cfg.name.find("Station") == 0) {
            cfg.name = "Station #" + std::to_string(cfg.id);
        }
    } else {
        nextId_ = std::max(nextId_, cfg.id + 1);
    }

    station->configure(cfg);
    stations_.push_back(std::move(station));

    fprintf(stdout, "[Distributor] Added station '%s' (ID: %d)\n",
            cfg.name.c_str(), cfg.id);

    return cfg.id;
}

void BroadcastDistributor::removeStation(int stationId) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = std::find_if(stations_.begin(), stations_.end(),
        [stationId](const auto& s) { return s->config().id == stationId; });

    if (it != stations_.end()) {
        fprintf(stdout, "[Distributor] Removed station '%s' (ID: %d)\n",
                (*it)->config().name.c_str(), stationId);
        (*it)->disconnect();
        stations_.erase(it);
        
        // If all stations are gone, reset the nextId counter
        if (stations_.empty()) {
            nextId_ = 1;
        }
    }
}

void BroadcastDistributor::updateStation(int stationId, const StationConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& station : stations_) {
        if (station->config().id == stationId) {
            // Check if we need to reconnect
            bool wasConnected = station->isConnected();
            
            if (wasConnected) {
                fprintf(stdout, "[Distributor] Asynchronously reconnecting station '%s' after config change...\n", config.name.c_str());
                
                // Use a separate thread to avoid blocking the UI thread during disconnect/connect
                // Note: We capture the station pointer and the new config.
                // In a production app, we'd use a more robust task queue, but for this refactor, 
                // a detached thread is the direct fix for the UI freeze.
                std::thread([s = station.get(), config]() {
                    s->disconnect();
                    s->configure(config);
                    s->connect();
                    fprintf(stdout, "[Distributor] Station '%s' reconnected successfully.\n", config.name.c_str());
                }).detach();
            } else {
                station->configure(config);
            }
            break;
        }
    }
}

int BroadcastDistributor::stationCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(stations_.size());
}

const StationConfig& BroadcastDistributor::stationConfig(int index) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return stations_[index]->config();
}

void BroadcastDistributor::connectStation(int stationId) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& station : stations_) {
        if (station->config().id == stationId) {
            station->connect();
            break;
        }
    }
}

void BroadcastDistributor::disconnectStation(int stationId) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& station : stations_) {
        if (station->config().id == stationId) {
            station->disconnect();
            break;
        }
    }
}

void BroadcastDistributor::connectAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& station : stations_) {
        if (station->config().enabled) {
            station->connect();
        }
    }
    fprintf(stdout, "[Distributor] Connect All: %zu stations\n", stations_.size());
}

void BroadcastDistributor::disconnectAll() {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& station : stations_) {
        station->disconnect();
    }
    fprintf(stdout, "[Distributor] Disconnect All\n");
}

// ─────────────────────────────────────────────
// Distribute — Called from mixer thread every frame
//
// This is the critical path. We copy the processed master buffer
// into each station's individual ring buffer. Each station thread
// then reads from its own ring buffer independently.
//
// This is the key architectural change from BUTT:
// BUTT had one stream_rb and one snd_stream_thread.
// We now have N ring buffers and N threads.
// ─────────────────────────────────────────────
void BroadcastDistributor::distribute(const float* buffer, int frames, int channels, int sampleRate) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& station : stations_) {
        if (station->isConnected()) {
            station->feedAudio(buffer, frames, channels, sampleRate);
        }
    }
}

StationStatus BroadcastDistributor::getStationStatus(int stationId) const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& station : stations_) {
        if (station->config().id == stationId) {
            return station->status();
        }
    }
    return StationStatus{};
}

void BroadcastDistributor::updateSongName(const std::string& songName) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& station : stations_) {
        if (station->isConnected()) {
            station->updateSongName(songName);
        }
    }
}

} // namespace ore
