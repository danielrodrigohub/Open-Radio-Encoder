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
    cfg.id = nextId_++;
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
        (*it)->disconnect();
        stations_.erase(it);
    }
}

void BroadcastDistributor::updateStation(int stationId, const StationConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& station : stations_) {
        if (station->config().id == stationId) {
            station->disconnect();
            station->configure(config);
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
void BroadcastDistributor::distribute(const float* buffer, int frames, int channels) {
    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& station : stations_) {
        if (station->isConnected()) {
            station->feedAudio(buffer, frames, channels);
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
