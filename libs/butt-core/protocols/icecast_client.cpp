// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Icecast Client Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "icecast_client.h"
#include <iostream>

#ifdef HAVE_SHOUT
#include <shout/shout.h>
#endif

namespace ore {

IcecastClient::IcecastClient() {
#ifdef HAVE_SHOUT
    shout_init();
#endif
}

IcecastClient::~IcecastClient() {
    disconnect();
#ifdef HAVE_SHOUT
    shout_shutdown();
#endif
}

int IcecastClient::connect(const IcecastConfig& cfg) {
#ifndef HAVE_SHOUT
    (void)cfg;
    std::cerr << "[IcecastClient] Built without libshout support" << std::endl;
    return -1;
#else
    disconnect();

    shout_ = shout_new();
    if (!shout_) {
        std::cerr << "[IcecastClient] Failed to allocate shout_t" << std::endl;
        return -1;
    }

    shout_set_host(shout_, cfg.addr.c_str());
    shout_set_port(shout_, cfg.port);
    shout_set_password(shout_, cfg.password.c_str());
    
    // Icecast 2 requires a mount point starting with /
    std::string mount = cfg.mount;
    if (mount.empty()) mount = "/stream";
    else if (mount[0] != '/') mount = "/" + mount;
    shout_set_mount(shout_, mount.c_str());

    // Default Icecast username is 'source'
    if (cfg.user.empty())
        shout_set_user(shout_, "source");
    else
        shout_set_user(shout_, cfg.user.c_str());

    shout_set_protocol(shout_, SHOUT_PROTOCOL_HTTP);
    shout_set_agent(shout_, "OpenRadioEncoder/1.0");

    if (cfg.tls) {
        shout_set_tls(shout_, SHOUT_TLS_AUTO);
    } else {
        shout_set_tls(shout_, SHOUT_TLS_DISABLED);
    }

    if (cfg.content_type == "audio/ogg" || cfg.content_type == "application/ogg") {
        shout_set_content_format(shout_, SHOUT_FORMAT_OGG, SHOUT_USAGE_AUDIO, nullptr);
    } else if (cfg.content_type == "audio/aac") {
        // Transparent mode for AAC
        shout_set_content_format(shout_, SHOUT_FORMAT_MP3, SHOUT_USAGE_AUDIO, nullptr);
    } else {
        shout_set_content_format(shout_, SHOUT_FORMAT_MP3, SHOUT_USAGE_AUDIO, nullptr);
    }

    // Set audio info to help Icecast/players
    shout_set_audio_info(shout_, SHOUT_AI_BITRATE, std::to_string(cfg.bitrate).c_str());
    shout_set_audio_info(shout_, SHOUT_AI_SAMPLERATE, std::to_string(cfg.samplerate).c_str());
    shout_set_audio_info(shout_, SHOUT_AI_CHANNELS, std::to_string(cfg.channels).c_str());

    if (shout_open(shout_) == SHOUTERR_SUCCESS) {
        isConnected_ = true;
        std::cout << "[IcecastClient] Connected to " << cfg.addr << ":" << cfg.port << cfg.mount << std::endl;
        return 0;
    } else {
        std::cerr << "[IcecastClient] Connection failed: " << shout_get_error(shout_) << std::endl;
        shout_free(shout_);
        shout_ = nullptr;
        return -1;
    }
#endif
}

int IcecastClient::send(const uint8_t* data, int len) {
#ifndef HAVE_SHOUT
    (void)data;
    (void)len;
    return -1;
#else
    if (!isConnected_ || !shout_) return -1;

    int ret = shout_send(shout_, data, len);
    if (ret != SHOUTERR_SUCCESS) {
        std::cerr << "[IcecastClient] Send error: " << shout_get_error(shout_) << " (code " << ret << ")" << std::endl;
        return -1;
    }
    
    shout_sync(shout_);
    return len;
#endif
}

int IcecastClient::updateSong(const std::string& song) {
#ifndef HAVE_SHOUT
    (void)song;
    return -1;
#else
    if (!isConnected_ || !shout_) return -1;
    
    shout_metadata_t* meta = shout_metadata_new();
    shout_metadata_add(meta, "song", song.c_str());
    int ret = shout_set_metadata_utf8(shout_, meta);
    shout_metadata_free(meta);
    
    if (ret != SHOUTERR_SUCCESS) {
        std::cerr << "[IcecastClient] Failed to update metadata (code " << ret << "): " 
                  << shout_get_error(shout_) << " [User: " << (shout_get_user(shout_) ? shout_get_user(shout_) : "none") << "]" << std::endl;
        return -1;
    }

    return 0;
#endif
}

int IcecastClient::getListenerCount() {
    // libshout does not natively fetch listener counts.
    // In a production app, we would make an HTTP GET to /admin/stats.xml or /status-json.xsl
    return -1;
}

void IcecastClient::disconnect() {
#ifdef HAVE_SHOUT
    if (isConnected_ && shout_) {
        shout_close(shout_);
        shout_free(shout_);
        shout_ = nullptr;
        isConnected_ = false;
        std::cout << "[IcecastClient] Disconnected" << std::endl;
    }
#else
    isConnected_ = false;
#endif
}

} // namespace ore
