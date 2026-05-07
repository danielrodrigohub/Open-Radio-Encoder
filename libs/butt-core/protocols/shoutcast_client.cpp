// ═══════════════════════════════════════════════════════════════════════
// Open Radio Encoder — Shoutcast Client Implementation
// ═══════════════════════════════════════════════════════════════════════
#include "shoutcast_client.h"
#include <shout/shout.h>
#include <iostream>

namespace ore {

ShoutcastClient::ShoutcastClient() {
    shout_init();
}

ShoutcastClient::~ShoutcastClient() {
    disconnect();
    shout_shutdown();
}

int ShoutcastClient::connect(const ShoutcastConfig& cfg) {
    disconnect();

    shout_ = shout_new();
    if (!shout_) {
        std::cerr << "[ShoutcastClient] Failed to allocate shout_t" << std::endl;
        return -1;
    }

    shout_set_host(shout_, cfg.addr.c_str());
    shout_set_port(shout_, cfg.port);
    shout_set_password(shout_, cfg.password.c_str());
    
    if (!cfg.mount.empty()) {
        shout_set_mount(shout_, cfg.mount.c_str());
#ifdef SHOUT_PROTOCOL_SHOUTCAST
        shout_set_protocol(shout_, SHOUT_PROTOCOL_SHOUTCAST); // SC v2
#else
        shout_set_protocol(shout_, SHOUT_PROTOCOL_HTTP);      // Fallback for SC v2
#endif
    } else {
        shout_set_protocol(shout_, SHOUT_PROTOCOL_ICY);       // SC v1
    }

    if (!cfg.user.empty()) {
        shout_set_user(shout_, cfg.user.c_str());
    }

    shout_set_agent(shout_, "OpenRadioEncoder/1.0");

    shout_set_tls(shout_, SHOUT_TLS_DISABLED);

    if (cfg.content_type == "audio/ogg" || cfg.content_type == "application/ogg") {
        shout_set_content_format(shout_, SHOUT_FORMAT_OGG, SHOUT_USAGE_AUDIO, nullptr);
    } else if (cfg.content_type == "audio/aac") {
        shout_set_content_format(shout_, SHOUT_FORMAT_MP3, SHOUT_USAGE_AUDIO, "aac");
    } else {
        shout_set_content_format(shout_, SHOUT_FORMAT_MP3, SHOUT_USAGE_AUDIO, nullptr);
    }

    shout_set_audio_info(shout_, SHOUT_AI_BITRATE, std::to_string(cfg.bitrate).c_str());
    shout_set_audio_info(shout_, SHOUT_AI_SAMPLERATE, std::to_string(cfg.samplerate).c_str());
    shout_set_audio_info(shout_, SHOUT_AI_CHANNELS, std::to_string(cfg.channels).c_str());

    if (shout_open(shout_) == SHOUTERR_SUCCESS) {
        isConnected_ = true;
        std::cout << "[ShoutcastClient] Connected to " << cfg.addr << ":" << cfg.port << std::endl;
        return 0;
    } else {
        std::cerr << "[ShoutcastClient] Connection failed: " << shout_get_error(shout_) << std::endl;
        shout_free(shout_);
        shout_ = nullptr;
        return -1;
    }
}

int ShoutcastClient::send(const uint8_t* data, int len) {
    if (!isConnected_ || !shout_) return -1;

    int ret = shout_send(shout_, data, len);
    if (ret != SHOUTERR_SUCCESS) {
        std::cerr << "[ShoutcastClient] Send error: " << shout_get_error(shout_) << std::endl;
        return -1;
    }
    
    shout_sync(shout_);
    return len;
}

int ShoutcastClient::updateSong(const std::string& song) {
    if (!isConnected_ || !shout_) return -1;
    
    shout_metadata_t* meta = shout_metadata_new();
    (void)shout_metadata_add(meta, "song", song.c_str());
    int ret = shout_set_metadata_utf8(shout_, meta);
    shout_metadata_free(meta);
    
    if (ret != SHOUTERR_SUCCESS) {
        std::cerr << "[ShoutcastClient] Failed to update metadata (code " << ret << "): " << shout_get_error(shout_) << std::endl;
        return -1;
    }
    return 0;
}

int ShoutcastClient::getListenerCount() {
    // Like Icecast, libshout doesn't provide listener counts directly.
    return -1;
}

void ShoutcastClient::disconnect() {
    if (isConnected_ && shout_) {
        shout_close(shout_);
        shout_free(shout_);
        shout_ = nullptr;
        isConnected_ = false;
        std::cout << "[ShoutcastClient] Disconnected" << std::endl;
    }
}

} // namespace ore
