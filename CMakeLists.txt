cmake_minimum_required(VERSION 3.22)
project(OpenRadioEncoder VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────
# JUCE — Add as subdirectory (git submodule)
# ─────────────────────────────────────────────
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    message(STATUS "JUCE not found. Run: git submodule add https://github.com/juce-framework/JUCE.git")
    message(STATUS "Continuing with stub configuration for code review...")
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
endif()

# ─────────────────────────────────────────────
# Find system libraries
# ─────────────────────────────────────────────
find_package(PkgConfig REQUIRED)

# Audio I/O
pkg_check_modules(PORTAUDIO portaudio-2.0)

# Encoders
pkg_check_modules(LAME IMPORTED_TARGET lame)
pkg_check_modules(TWOLAME IMPORTED_TARGET twolame)
pkg_check_modules(OPUS IMPORTED_TARGET opus)
pkg_check_modules(VORBIS IMPORTED_TARGET vorbis vorbisenc)
pkg_check_modules(FLAC IMPORTED_TARGET flac)
pkg_check_modules(FDK_AAC IMPORTED_TARGET fdk-aac)
pkg_check_modules(SAMPLERATE IMPORTED_TARGET samplerate)

# Streaming
pkg_check_modules(SHOUT IMPORTED_TARGET shout)

# TLS
find_package(OpenSSL)

# ─────────────────────────────────────────────
# BUTT Core Engine — Static Library
# ─────────────────────────────────────────────
add_subdirectory(libs/butt-core)

# ─────────────────────────────────────────────
# Main Application (JUCE)
# ─────────────────────────────────────────────
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    juce_add_gui_app(OpenRadioEncoder
        PRODUCT_NAME "Open Radio Encoder"
        VERSION "1.0.0"
        COMPANY_NAME "OpenRadioProject"
        BUNDLE_ID "com.openradio.encoder"
        ICON_BIG "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/appicon.png"
    )

    target_sources(OpenRadioEncoder PRIVATE
        # Entry point
        src/Main.cpp

        # Engine
        src/engine/AudioPipeline.cpp
        src/engine/AudioPipeline.h
        src/engine/BroadcastDistributor.cpp
        src/engine/BroadcastDistributor.h
        src/engine/StationConnection.cpp
        src/engine/StationConnection.h
        src/engine/RecordingEngine.cpp
        src/engine/RecordingEngine.h
        src/engine/VST3HostProcessor.cpp
        src/engine/VST3HostProcessor.h

        # UI
        src/ui/MainWindow.cpp
        src/ui/MainWindow.h
        src/ui/LookAndFeel_OpenRadio.cpp
        src/ui/LookAndFeel_OpenRadio.h
        src/ui/ToolbarComponent.cpp
        src/ui/ToolbarComponent.h
        src/ui/StreamingPanel.cpp
        src/ui/StreamingPanel.h
        src/ui/StationBlock.cpp
        src/ui/StationBlock.h
        src/ui/CurrentSongPanel.cpp
        src/ui/CurrentSongPanel.h
        src/ui/RecordingPanel.cpp
        src/ui/RecordingPanel.h
        src/ui/VUMeterComponent.cpp
        src/ui/VUMeterComponent.h
    )

    target_include_directories(OpenRadioEncoder PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/butt-core
    )

    target_compile_definitions(OpenRadioEncoder PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_PLUGINHOST_VST3=1
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:OpenRadioEncoder,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:OpenRadioEncoder,JUCE_VERSION>"
    )

    target_link_libraries(OpenRadioEncoder PRIVATE
        butt-core
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_opengl
    )
else()
    # Fallback: Build without JUCE for CI/testing
    message(WARNING "Building without JUCE - engine library only")
endif()
