#include "ManagerDialogs.h"
#include "ui/LookAndFeel_OpenRadio.h"
#include <cstdio>
#include <cmath>

namespace ore {

StationEditor::StationEditor() {
    auto setupEdit = [this](juce::TextEditor& e, const juce::String& text) {
        e.setFont(juce::Font(juce::FontOptions(16.0f)));
        e.setJustification(juce::Justification::centredLeft);
        e.setText(text);
        addAndMakeVisible(e);
    };
    setupEdit(nameEdit_, "Station #1");
    setupEdit(addressEdit_, "localhost");
    setupEdit(portEdit_, "8000");
    setupEdit(mountEdit_, "/stream");
    setupEdit(userEdit_, "source");
    setupEdit(passEdit_, "");

    serverTypeBox_.addItem("Icecast", 1);
    serverTypeBox_.addItem("Shoutcast", 2);
    serverTypeBox_.setSelectedId(1);
    addAndMakeVisible(serverTypeBox_);

    codecBox_.addItem("MP3", 1);
    codecBox_.addItem("MP2", 2);
    codecBox_.addItem("AAC-HE", 3);
    codecBox_.addItem("Opus", 4);
    codecBox_.addItem("Vorbis", 5);
    codecBox_.addItem("FLAC", 6);
    codecBox_.setSelectedId(1);
    addAndMakeVisible(codecBox_);

    for (int sr : {22050, 44100, 48000, 96000}) samplerateBox_.addItem(juce::String(sr), sr);
    samplerateBox_.setSelectedId(44100);
    addAndMakeVisible(samplerateBox_);

    channelsBox_.addItem("Mono", 1);
    channelsBox_.addItem("Stereo", 2);
    channelsBox_.setSelectedId(2);
    addAndMakeVisible(channelsBox_);

    for (int br : {32, 48, 64, 96, 128, 160, 192, 224, 256, 320})
        bitrateBox_.addItem(juce::String(br) + " kbps", br);
    bitrateBox_.setSelectedId(128);
    addAndMakeVisible(bitrateBox_);

    for (int sec : {1, 5, 10, 15, 30, 45, 60}) 
        reconnectIntervalBox_.addItem(juce::String(sec) + "s", sec);
    reconnectIntervalBox_.setSelectedId(5);
    addAndMakeVisible(reconnectIntervalBox_);

    addAndMakeVisible(tlsToggle_);
    setSize(360, 460);
}

void StationEditor::loadFrom(const StationConfig& c) {
    auto updateEdit = [](juce::TextEditor& e, const juce::String& text) {
        e.setFont(juce::Font(juce::FontOptions(16.0f)));
        e.setJustification(juce::Justification::centredLeft);
        e.setText(text);
    };
    updateEdit(nameEdit_, c.name);
    updateEdit(addressEdit_, c.address);
    updateEdit(portEdit_, juce::String(c.port));
    updateEdit(mountEdit_, c.mountPoint);
    updateEdit(userEdit_, c.username);
    updateEdit(passEdit_, c.password);
    
    bitrateBox_.setSelectedId(c.encoder.bitrate);
    serverTypeBox_.setSelectedId(c.serverType == ServerType::Icecast ? 1 : 2);
    codecBox_.setSelectedId(static_cast<int>(c.encoder.codec) + 1);
    int sr = c.encoder.samplerate;
    if (sr == 44000) sr = 44100;
    samplerateBox_.setSelectedId(sr);
    channelsBox_.setSelectedId(c.encoder.channels);
    reconnectIntervalBox_.setSelectedId(c.reconnectInterval);
    tlsToggle_.setToggleState(c.useTLS, juce::dontSendNotification);
}

StationConfig StationEditor::toConfig() const {
    StationConfig c;
    c.name = nameEdit_.getText().toStdString();
    c.serverType = serverTypeBox_.getSelectedId() == 1 ? ServerType::Icecast : ServerType::Shoutcast;
    c.address = addressEdit_.getText().toStdString();
    c.port = portEdit_.getText().getIntValue();
    c.mountPoint = mountEdit_.getText().toStdString();
    c.username = userEdit_.getText().toStdString();
    c.password = passEdit_.getText().toStdString();
    c.useTLS = tlsToggle_.getToggleState();
    c.encoder.codec = static_cast<CodecType>(codecBox_.getSelectedId() - 1);
    c.encoder.samplerate = samplerateBox_.getSelectedId();
    c.encoder.channels = channelsBox_.getSelectedId();
    c.encoder.bitrate = bitrateBox_.getSelectedId();
    if (c.encoder.bitrate <= 0) c.encoder.bitrate = 128;
    c.reconnectInterval = reconnectIntervalBox_.getSelectedId();
    return c;
}

void StationEditor::resized() {
    auto b = getLocalBounds().reduced(12, 6);
    int half = (b.getWidth() - 4) / 2;
    int lblH = 14, editH = 38, sep = 2;

    auto makeRow2 = [&](juce::Component& a, juce::Component& b2) {
        auto r = b.removeFromTop(lblH + editH + sep);
        r.removeFromTop(lblH + sep);
        a.setBounds(r.removeFromLeft(half));
        r.removeFromLeft(6);
        b2.setBounds(r);
        b.removeFromTop(6);
    };

    makeRow2(nameEdit_, addressEdit_);
    makeRow2(portEdit_, mountEdit_);
    makeRow2(userEdit_, passEdit_);
    makeRow2(serverTypeBox_, codecBox_);
    makeRow2(samplerateBox_, channelsBox_);

    auto r3 = b.removeFromTop(lblH + editH + sep);
    r3.removeFromTop(lblH + sep);
    bitrateBox_.setBounds(r3.removeFromLeft(half));
    r3.removeFromLeft(6);
    reconnectIntervalBox_.setBounds(r3);
    
    b.removeFromTop(6);
    tlsToggle_.setBounds(b.removeFromTop(26));
}

void StationEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(LookAndFeel_OpenRadio::kSurface));
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kBorder));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);

    g.setFont(juce::Font(juce::FontOptions(14.0f))); // Slightly smaller for labels
    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kTextSecondary));
    auto b = getLocalBounds().reduced(12, 6);
    int half = (b.getWidth() - 4) / 2;
    int lblH = 14, editH = 38, sep = 2;

    int y = b.getY();
    auto drawLabels = [&](const juce::String& l1, const juce::String& l2) {
        g.drawText(l1, b.getX(), y, half, lblH, juce::Justification::centredLeft, false);
        if (l2.isNotEmpty())
            g.drawText(l2, b.getX() + half + 6, y, half, lblH, juce::Justification::centredLeft, false);
        y += lblH + editH + sep + 6;
    };

    drawLabels("Name", "Address");
    drawLabels("Port", "Mount");
    drawLabels("Username", "Password");
    drawLabels("Server Type", "Codec");
    drawLabels("Sample Rate", "Channels");
    drawLabels("Bitrate (kbps)", "Reconnect Every");
}

ManagerDialog::ManagerDialog(BroadcastDistributor* dist)
    : dist_(dist) {

    header_.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    addAndMakeVisible(header_);

    stationList_.setRowHeight(30);
    addAndMakeVisible(stationList_);
    addAndMakeVisible(editor_);

    addBtn_.onClick = [this]() { addStation(); };
    applyBtn_.onClick = [this]() { applyCurrent(); };
    deleteBtn_.onClick = [this]() { deleteCurrent(); };
    addAndMakeVisible(addBtn_);
    addAndMakeVisible(applyBtn_);
    addAndMakeVisible(deleteBtn_);

    connectBtn_.onClick = [this]() {
        if (dist_ && onConnectStation && currentEdit_ >= 0)
            onConnectStation(currentEdit_);
    };
    disconnectBtn_.onClick = [this]() {
        if (dist_ && onDisconnectStation && currentEdit_ >= 0)
            onDisconnectStation(currentEdit_);
    };
    addAndMakeVisible(connectBtn_);
    addAndMakeVisible(disconnectBtn_);

    if (dist_) {
        for (int i = 0; i < dist_->stationCount(); i++) {
            stationIds_.push_back(dist_->stationConfig(i).id);
        }
    }
    stationList_.updateContent();

    setSize(720, 480);
}

void ManagerDialog::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour(LookAndFeel_OpenRadio::kBackground));
}

void ManagerDialog::resized() {
    auto b = getLocalBounds().reduced(12);
    header_.setBounds(b.removeFromTop(28));
    b.removeFromTop(8);

    auto btnRow = b.removeFromTop(32);
    addBtn_.setBounds(btnRow.removeFromLeft(70));
    btnRow.removeFromLeft(6);
    applyBtn_.setBounds(btnRow.removeFromLeft(70));
    btnRow.removeFromLeft(6);
    deleteBtn_.setBounds(btnRow.removeFromLeft(70));
    btnRow.removeFromLeft(12);
    disconnectBtn_.setBounds(btnRow.removeFromRight(90));
    btnRow.removeFromRight(6);
    connectBtn_.setBounds(btnRow.removeFromRight(90));

    b.removeFromTop(8);

    stationList_.setBounds(b.removeFromLeft(200));
    b.removeFromLeft(8);
    editor_.setBounds(b);
}

std::string ManagerDialog::formatStationStatus(int stationId) const {
    if (!dist_) return "?";
    auto status = dist_->getStationStatus(stationId);
    switch (status.state) {
        case StationState::Connected: {
            int h = static_cast<int>(status.streamTimeSecs) / 3600;
            int m = (static_cast<int>(status.streamTimeSecs) % 3600) / 60;
            int s = static_cast<int>(status.streamTimeSecs) % 60;
            char buf[32];
            snprintf(buf, sizeof(buf), "ON AIR %02d:%02d:%02d", h, m, s);
            return buf;
        }
        case StationState::Connecting:  return "Connecting...";
        case StationState::Reconnecting: return "Reconnecting...";
        case StationState::Error:       return "Error: " + status.errorMessage;
        case StationState::Disconnected:
        default:                         return "Disconnected";
    }
}

void ManagerDialog::paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool selected) {
    g.setColour(selected ? juce::Colour(LookAndFeel_OpenRadio::kSurfaceHover)
                         : juce::Colour(LookAndFeel_OpenRadio::kSurface));
    g.fillRoundedRectangle(1.0f, 1.0f, static_cast<float>(w) - 2, static_cast<float>(h) - 2, 3.0f);

    if (dist_ && row < static_cast<int>(stationIds_.size())) {
        int id = stationIds_[static_cast<size_t>(row)];
        for (int i = 0; i < dist_->stationCount(); i++) {
            if (dist_->stationConfig(i).id == id) {
                g.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
                g.setColour(juce::Colour(LookAndFeel_OpenRadio::kTextPrimary));
                g.drawText(dist_->stationConfig(i).name, 8, 0, w / 2 - 8, h,
                          juce::Justification::centredLeft, true);

                std::string statusText = formatStationStatus(id);
                g.setFont(juce::Font(juce::FontOptions(11.0f)));
                auto s = dist_->getStationStatus(id);
                if (s.state == StationState::Connected)
                    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kAccentGreen));
                else if (s.state == StationState::Connecting || s.state == StationState::Reconnecting)
                    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kVUYellow));
                else if (s.state == StationState::Error)
                    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kAccentRed));
                else
                    g.setColour(juce::Colour(LookAndFeel_OpenRadio::kTextDim));
                g.drawText(statusText, w / 2, 0, w / 2 - 8, h,
                          juce::Justification::centredRight, true);
                break;
            }
        }
    }
}

void ManagerDialog::selectStation(int row) {
    if (!dist_ || row < 0 || row >= static_cast<int>(stationIds_.size())) return;
    for (int i = 0; i < dist_->stationCount(); i++) {
        if (dist_->stationConfig(i).id == stationIds_[static_cast<size_t>(row)]) {
            currentEdit_ = dist_->stationConfig(i).id;
            editor_.loadFrom(dist_->stationConfig(i));
            break;
        }
    }
}

void ManagerDialog::selectStationById(int stationId) {
    if (!dist_) return;
    for (int i = 0; i < static_cast<int>(stationIds_.size()); i++) {
        if (stationIds_[static_cast<size_t>(i)] == stationId) {
            stationList_.selectRow(i);
            selectStation(i);
            break;
        }
    }
}

void ManagerDialog::addStation() {
    if (!dist_) return;
    StationConfig sc = editor_.toConfig();
    sc.id = 0;
    int newId = dist_->addStation(sc);
    stationIds_.push_back(newId);
    currentEdit_ = newId;
    stationList_.updateContent();
    stationList_.repaint();

    for (int i = 0; i < dist_->stationCount(); i++) {
        if (dist_->stationConfig(i).id == newId) {
            editor_.loadFrom(dist_->stationConfig(i));
            if (onStationAdded) onStationAdded(dist_->stationConfig(i));
            break;
        }
    }
}

void ManagerDialog::applyCurrent() {
    if (!dist_ || currentEdit_ < 0) return;
    StationConfig c = editor_.toConfig();
    c.id = currentEdit_;
    dist_->updateStation(currentEdit_, c);
    stationList_.updateContent();
    stationList_.repaint();
    if (onStationsChanged) onStationsChanged();
}

void ManagerDialog::deleteCurrent() {
    if (!dist_ || currentEdit_ < 0) return;
    dist_->removeStation(currentEdit_);
    stationIds_.erase(std::remove(stationIds_.begin(), stationIds_.end(), currentEdit_),
                      stationIds_.end());
    currentEdit_ = -1;
    stationList_.updateContent();
    stationList_.repaint();
    if (onStationsChanged) onStationsChanged();
}

void ManagerDialog::refreshFromDistributor() {
    stationIds_.clear();
    if (dist_) {
        for (int i = 0; i < dist_->stationCount(); i++) {
            stationIds_.push_back(dist_->stationConfig(i).id);
        }
    }
    stationList_.updateContent();
    stationList_.repaint();
}

} // namespace ore
