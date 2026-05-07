#include "MainWindow.h"
#include "LookAndFeel_OpenRadio.h"
#include "engine/RecordingEngine.h"
#include "config/ConfigManager.h"
#include <cstdio>

namespace ore {

MainContentComponent::MainContentComponent() {
    addAndMakeVisible(toolbar_);

    addChildComponent(streamingPanel_);
    addChildComponent(currentSongPanel_);
    addChildComponent(recordingPanel_);
    addChildComponent(vuMeter_);

    setupEngine();
    wireUI();
    showTab(0);

    startTimerHz(30);
    setSize(980, 700);
}

MainContentComponent::~MainContentComponent() {
    stopTimer();
    if (pipeline_) pipeline_->stop();
}

void MainContentComponent::showTab(int index) {
    currentTab_ = index;

    bool streaming = (index == 0);
    streamingPanel_.setVisible(streaming);
    currentSongPanel_.setVisible(streaming);
    recordingPanel_.setVisible(streaming);
    vuMeter_.setVisible(streaming);

    if (settingsView_) settingsView_->setVisible(index == 1);
    if (schedulerView_) schedulerView_->setVisible(index == 2);
    if (stationMgrView_) {
        stationMgrView_->setVisible(index == 3);
        if (index == 3) stationMgrView_->refreshFromDistributor();
    }
    if (consoleLogView_) consoleLogView_->setVisible(index == 4);
    if (audioFxView_) audioFxView_->setVisible(index == 5);
    if (mixerView_) mixerView_->setVisible(index == 6);

    toolbar_.setSelectedTab(index);
    resized();
}

void MainContentComponent::setupEngine() {
    pipeline_ = std::make_unique<AudioPipeline>();
    PipelineConfig cfg;
    cfg.sampleRate = 44100;
    cfg.channels = 2;
    cfg.bufferSize = 1024;
    cfg.deviceIndex = -1;

    int rc = pipeline_->initialize(cfg);
    if (rc == 0) {
        pipeline_->start();
        engineRunning_ = true;
        fprintf(stdout, "[Main] Audio engine started\n");
    } else {
        fprintf(stderr, "[Main] Audio engine failed to initialize\n");
        engineRunning_ = false;
    }
}

void MainContentComponent::wireUI() {
    auto* dist = pipeline_ ? pipeline_->distributor() : nullptr;

    settingsView_ = std::make_unique<SettingsDialog>(pipeline_.get());
    settingsView_->onSettingsChanged = [this]() {
        fprintf(stdout, "[Main] Settings applied, audio pipeline restarted\n");
    };
    addChildComponent(settingsView_.get());

    schedulerView_ = std::make_unique<SchedulerDialog>();
    schedulerView_->onConnectAll = [this]() {
        if (auto* d = pipeline_->distributor()) d->connectAll();
    };
    schedulerView_->onDisconnectAll = [this]() {
        if (auto* d = pipeline_->distributor()) d->disconnectAll();
    };
    schedulerView_->onStartRecording = [this]() { recordingPanel_.onStartRecording(recordingPanel_.currentPath(), recordingPanel_.currentCodec(), recordingPanel_.currentQuality()); };
    schedulerView_->onStopRecording = [this]() { recordingPanel_.onStopRecording(); };
    addChildComponent(schedulerView_.get());

    stationMgrView_ = std::make_unique<ManagerDialog>(dist);
    stationMgrView_->onStationsChanged = [this]() {
        rebuildStreamingBlocks();
        saveAllStations(pipeline_->distributor());
        resized();
    };
    stationMgrView_->onStationAdded = [this](const StationConfig& sc) {
        addStationBlock(sc);
        saveAllStations(pipeline_->distributor());
        resized();
    };
    stationMgrView_->onConnectStation = [this](int stationId) {
        if (auto* d = pipeline_->distributor()) {
            d->connectStation(stationId);
            fprintf(stdout, "[Main] Station Manager: Connect station %d\n", stationId);
        }
    };
    stationMgrView_->onDisconnectStation = [this](int stationId) {
        if (auto* d = pipeline_->distributor()) {
            d->disconnectStation(stationId);
            fprintf(stdout, "[Main] Station Manager: Disconnect station %d\n", stationId);
        }
    };
    addChildComponent(stationMgrView_.get());

    consoleLogView_ = std::make_unique<ConsoleLogComponent>();
    addChildComponent(consoleLogView_.get());

    audioFxView_ = std::make_unique<AudioFxDialog>(pipeline_.get());
    addChildComponent(audioFxView_.get());

    mixerView_ = std::make_unique<MixerDialog>(pipeline_.get());
    addChildComponent(mixerView_.get());

    toolbar_.onButtonClicked = [this](int index) { showTab(index); };

    auto& cfg = ConfigManager::getInstance();
    auto stations = cfg.load();
    if (stations.empty()) {
        StationConfig sc1;
        sc1.id = 1; sc1.name = "Station #1";
        sc1.serverType = ServerType::Icecast;
        sc1.address = "localhost"; sc1.port = 8000;
        sc1.mountPoint = "/stream";
        sc1.username = "source"; sc1.password = "hackme";
        sc1.encoder.codec = CodecType::AAC;
        sc1.encoder.samplerate = 44100;
        sc1.encoder.channels = 2; sc1.encoder.bitrate = 128;
        stations.push_back(sc1);

        StationConfig sc2;
        sc2.id = 2; sc2.name = "Station #2";
        sc2.serverType = ServerType::Shoutcast;
        sc2.address = "localhost"; sc2.port = 8001;
        sc2.password = "changeme";
        sc2.encoder.codec = CodecType::MP3;
        sc2.encoder.samplerate = 44100;
        sc2.encoder.channels = 2; sc2.encoder.bitrate = 128;
        stations.push_back(sc2);
    }
    for (auto& sc : stations) {
        if (dist) dist->addStation(sc);
        addStationBlock(sc);
    }
    if (dist) saveAllStations(dist);

    streamingPanel_.onConnectAll = [this, dist]() { if (dist) dist->connectAll(); };
    streamingPanel_.onDisconnectAll = [this, dist]() { if (dist) dist->disconnectAll(); };
    streamingPanel_.onAddStation = [this, dist]() {
        if (!dist) return;
        StationConfig sc;
        sc.serverType = ServerType::Icecast;
        sc.address = "localhost"; sc.port = 8000;
        sc.mountPoint = "/stream";
        sc.username = "source"; sc.password = "hackme";
        sc.encoder.codec = CodecType::MP3;
        sc.encoder.samplerate = 44100;
        sc.encoder.channels = 2;
        sc.encoder.bitrate = 128;

        int newId = dist->addStation(sc);
        // Fetch the updated config (with correct ID and name) from the distributor
        for (int i = 0; i < dist->stationCount(); i++) {
            if (dist->stationConfig(i).id == newId) {
                addStationBlock(dist->stationConfig(i));
                break;
            }
        }

        saveAllStations(dist);
        resized();
        if (stationMgrView_) {
            stationMgrView_->refreshFromDistributor();
        }
    };

    currentSongPanel_.onUpdateSong = [this, dist](const juce::String& song) {
        if (dist) dist->updateSongName(song.toStdString());
    };

    recordingPanel_.onStartRecording = [this](const juce::String& path, const juce::String& codec, int quality) {
        auto* rec = pipeline_ ? pipeline_->recordingEngine() : nullptr;
        if (rec) {
            juce::File dir(path);
            dir.getParentDirectory().createDirectory();
            int rc = rec->startRecording(path.toStdString(), codec.toStdString(), 44100, 2, quality);
            if (rc == 0) {
                recordingPanel_.setRecordingActive(true);
                recordingPanel_.setRecordingPath(path);
            }
        }
    };
    recordingPanel_.onStopRecording = [this]() {
        auto* rec = pipeline_ ? pipeline_->recordingEngine() : nullptr;
        if (rec) {
            rec->stopRecording();
            recordingPanel_.setRecordingActive(false);
        }
    };

    // ── AUTO-START ──
    if (dist) {
        dist->connectAll();
        // Force branding metadata immediately after launch
        dist->updateSongName("Open Radio Encoder - Made in Chile with Love!");
    }
    if (pipeline_) pipeline_->start();

    // Final setup
    currentTab_ = 0;
    toolbar_.setSelectedTab(0);
    showTab(0);
}

void MainContentComponent::addStationBlock(const StationConfig& sc) {
    juce::String serverType = (sc.serverType == ServerType::Icecast) ? "Icecast" : "Shoutcast";
    streamingPanel_.addStation(sc.id, sc.name, serverType, sc.encoder.codecName());

    auto* block = streamingPanel_.stationBlocks().back().get();
    int stationId = sc.id;
    auto* dist = pipeline_->distributor();

    block->onConnect = [dist, stationId]() { if (dist) dist->connectStation(stationId); };
    block->onDisconnect = [dist, stationId]() { if (dist) dist->disconnectStation(stationId); };
    block->onDelete = [this, dist, stationId]() {
        if (dist) dist->removeStation(stationId);
        rebuildStreamingBlocks();
        saveAllStations(dist);
        resized();
    };
    block->onEdit = [this, stationId]() {
        showTab(3);
        if (stationMgrView_) stationMgrView_->selectStationById(stationId);
    };
}

void MainContentComponent::rebuildStreamingBlocks() {
    auto* dist = pipeline_ ? pipeline_->distributor() : nullptr;
    streamingPanel_.clearStations();

    if (!dist) return;

    for (int i = 0; i < dist->stationCount(); i++) {
        auto& sc = dist->stationConfig(i);
        juce::String serverType = (sc.serverType == ServerType::Icecast) ? "Icecast" : "Shoutcast";
        streamingPanel_.addStation(sc.id, sc.name, serverType, sc.encoder.codecName());

        auto* block = streamingPanel_.stationBlocks().back().get();
        int stationId = sc.id;

        block->onConnect = [dist, stationId]() { if (dist) dist->connectStation(stationId); };
        block->onDisconnect = [dist, stationId]() { if (dist) dist->disconnectStation(stationId); };
        block->onDelete = [this, dist, stationId]() {
            if (dist) dist->removeStation(stationId);
            rebuildStreamingBlocks();
            saveAllStations(dist);
            resized();
        };
        block->onEdit = [this, stationId]() {
            showTab(3);
            if (stationMgrView_) stationMgrView_->selectStationById(stationId);
        };
    }

    if (stationMgrView_) stationMgrView_->refreshFromDistributor();
}

void MainContentComponent::saveAllStations(BroadcastDistributor* dist) {
    if (dist == nullptr) return;
    std::vector<StationConfig> configs;
    for (int i = 0; i < dist->stationCount(); i++)
        configs.push_back(dist->stationConfig(i));
    ConfigManager::getInstance().save(configs);
}

void MainContentComponent::timerCallback() {
    currentSongPanel_.pollFileIfNeeded();

    if (pipeline_) {
        // Calculate CPU usage (Real-time Audio Load)
        float cpu = 0.0f;
        if (pipeline_->isRunning()) {
            cpu = pipeline_->cpuUsage() * 100.0f;
        }
        toolbar_.setCpuUsage(cpu);

        auto& vu = pipeline_->vuMeterData();
        vuMeter_.setLevels(vu.peakL.load(), vu.peakR.load(),
                           vu.rmsL.load(), vu.rmsR.load());

        auto* rec = pipeline_->recordingEngine();
        if (rec && rec->isRecording()) {
            double kb = rec->kbytesWritten();
            char buf[32];
            snprintf(buf, sizeof(buf), kb > 1024.0 ? "%.2f MB" : "%.1f KB",
                     kb > 1024.0 ? kb / 1024.0 : kb);
            recordingPanel_.setRecordingSize(buf);

            double secs = rec->durationSecs();
            int m = static_cast<int>(secs) / 60;
            int s = static_cast<int>(secs) % 60;
            char ebuf[8];
            snprintf(ebuf, sizeof(ebuf), "%02d:%02d", m, s);
            recordingPanel_.setRecordingElapsed(ebuf);
        }
    }

    auto* dist = pipeline_ ? pipeline_->distributor() : nullptr;
    if (dist) {
        for (auto& block : streamingPanel_.stationBlocks()) {
            int id = block->stationId();
            auto status = dist->getStationStatus(id);
            block->setConnected(status.state == StationState::Connected);

            if (status.state == StationState::Connected) {
                int h = static_cast<int>(status.streamTimeSecs) / 3600;
                int m = (static_cast<int>(status.streamTimeSecs) % 3600) / 60;
                int s = static_cast<int>(status.streamTimeSecs) % 60;
                char buf[16];
                snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
                block->setStreamTime(buf);
            } else if (status.state == StationState::Connecting) {
                block->setStreamStatus("Connecting...");
            } else if (status.state == StationState::Reconnecting) {
                block->setStreamStatus("Reconnecting...");
            } else if (status.state == StationState::Error) {
                block->setStreamStatus("Error");
            }
            block->setListenerCount(status.listeners);
        }
    }
}

void MainContentComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(LookAndFeel_OpenRadio::kBackground));
}

void MainContentComponent::resized() {
    auto bounds = getLocalBounds();
    toolbar_.setBounds(bounds.removeFromTop(70));

    if (currentTab_ == 0) {
        auto vuArea = bounds.removeFromRight(80);
        vuMeter_.setBounds(vuArea.reduced(5));

        auto left = bounds.reduced(10, 5);
        auto streamingH = left.getHeight() * 50 / 100;
        streamingPanel_.setBounds(left.removeFromTop(streamingH));
        left.removeFromTop(4);
        currentSongPanel_.setBounds(left.removeFromTop(100));
        left.removeFromTop(4);
        recordingPanel_.setBounds(left);
    } else {
        auto tabBounds = bounds.reduced(10, 10);
        if (currentTab_ == 1 && settingsView_) settingsView_->setBounds(tabBounds);
        else if (currentTab_ == 2 && schedulerView_) schedulerView_->setBounds(tabBounds);
        else if (currentTab_ == 3 && stationMgrView_) stationMgrView_->setBounds(tabBounds);
        else if (currentTab_ == 4 && consoleLogView_) consoleLogView_->setBounds(tabBounds);
        else if (currentTab_ == 5 && audioFxView_) audioFxView_->setBounds(tabBounds);
        else if (currentTab_ == 6 && mixerView_) mixerView_->setBounds(tabBounds);
    }
}

MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name, juce::Colour(LookAndFeel_OpenRadio::kBackground),
                     DocumentWindow::allButtons) {
    setUsingNativeTitleBar(true);
    setContentOwned(new MainContentComponent(), true);
    setResizable(true, true);
    setResizeLimits(800, 600, 1600, 1200);
    centreWithSize(getWidth(), getHeight());
    setVisible(true);
}

void MainWindow::closeButtonPressed() {
    juce::JUCEApplication::getInstance()->systemRequestedQuit();
}

} // namespace ore
