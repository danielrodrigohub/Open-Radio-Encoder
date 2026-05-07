cmake_minimum_required(VERSION 3.22)

# ─────────────────────────────────────────────
# BUTT Core Engine — Static Library
# Refactored audio/streaming engine from BUTT 1.46.0
# ─────────────────────────────────────────────
add_library(butt-core STATIC
    # Encoders
    encoders/encoder_interface.h
    encoders/lame_encoder.cpp
    encoders/lame_encoder.h
    encoders/twolame_encoder.cpp
    encoders/twolame_encoder.h
    encoders/aac_encoder.cpp
    encoders/aac_encoder.h
    encoders/opus_encoder.cpp
    encoders/opus_encoder.h
    encoders/vorbis_encoder.cpp
    encoders/vorbis_encoder.h
    encoders/flac_encoder.cpp
    encoders/flac_encoder.h

    # Protocols
    protocols/icecast_client.cpp
    protocols/icecast_client.h
    protocols/shoutcast_client.cpp
    protocols/shoutcast_client.h

    # DSP
    dsp/dsp_effects.cpp
    dsp/dsp_effects.h
    dsp/biquad.cpp
    dsp/biquad.h

    # Networking
    network/sockfuncs.cpp
    network/sockfuncs.h
    network/tls_wrapper.cpp
    network/tls_wrapper.h

    # Utilities
    utils/ringbuffer.cpp
    utils/ringbuffer.h
    utils/timer_utils.cpp
    utils/timer_utils.h
    utils/cJSON.cpp
    utils/cJSON.h
    utils/str_utils.cpp
    utils/str_utils.h
    utils/uri_encode.cpp
    utils/uri_encode.h
)

target_include_directories(butt-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/encoders
    ${CMAKE_CURRENT_SOURCE_DIR}/protocols
    ${CMAKE_CURRENT_SOURCE_DIR}/dsp
    ${CMAKE_CURRENT_SOURCE_DIR}/network
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
)

if(PORTAUDIO_FOUND)
    target_link_libraries(butt-core PUBLIC PORTAUDIO::portaudio)
    target_compile_definitions(butt-core PUBLIC HAVE_PORTAUDIO)
endif()

if(SHOUT_FOUND)
    target_link_libraries(butt-core PUBLIC SHOUT::shout)
    target_compile_definitions(butt-core PUBLIC HAVE_SHOUT)
endif()

if(LAME_FOUND)
    target_link_libraries(butt-core PUBLIC LAME::lame)
    target_compile_definitions(butt-core PUBLIC HAVE_LAME)
endif()

if(TWOLAME_FOUND)
    target_link_libraries(butt-core PUBLIC PkgConfig::TWOLAME)
    target_compile_definitions(butt-core PUBLIC HAVE_TWOLAME)
endif()

if(OPUS_FOUND)
    target_link_libraries(butt-core PUBLIC PkgConfig::OPUS PkgConfig::OGG)
    target_compile_definitions(butt-core PUBLIC HAVE_OPUS)
endif()

if(VORBIS_FOUND)
    target_link_libraries(butt-core PUBLIC PkgConfig::VORBIS PkgConfig::OGG)
    target_compile_definitions(butt-core PUBLIC HAVE_VORBIS)
endif()

if(FLAC_FOUND)
    target_link_libraries(butt-core PUBLIC PkgConfig::FLAC)
    target_compile_definitions(butt-core PUBLIC HAVE_FLAC)
endif()

if(FDK_AAC_FOUND)
    target_link_libraries(butt-core PUBLIC PkgConfig::FDK_AAC)
    target_compile_definitions(butt-core PUBLIC HAVE_LIBFDK_AAC)
endif()

if(SAMPLERATE_FOUND)
    target_link_libraries(butt-core PUBLIC PkgConfig::SAMPLERATE)
endif()

if(OPENSSL_FOUND)
    target_link_libraries(butt-core PUBLIC OpenSSL::SSL OpenSSL::Crypto)
    target_compile_definitions(butt-core PUBLIC HAVE_LIBSSL)
endif()

# Platform-specific
if(APPLE)
    target_link_libraries(butt-core PUBLIC "-framework CoreAudio" "-framework AudioToolbox")
elseif(WIN32)
    target_link_libraries(butt-core PUBLIC ws2_32)
endif()
