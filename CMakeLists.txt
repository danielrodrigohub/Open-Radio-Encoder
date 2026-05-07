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

# Audio
find_library(PORTAUDIO_LIBRARY NAMES portaudio)
find_path(PORTAUDIO_INCLUDE_DIR NAMES portaudio.h)
if(PORTAUDIO_LIBRARY AND PORTAUDIO_INCLUDE_DIR)
    add_library(PORTAUDIO::portaudio UNKNOWN IMPORTED)
    set_target_properties(PORTAUDIO::portaudio PROPERTIES
        IMPORTED_LOCATION "${PORTAUDIO_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${PORTAUDIO_INCLUDE_DIR}"
    )
    set(PORTAUDIO_FOUND TRUE)
endif()

# Encoders
find_library(LAME_LIBRARY NAMES mp3lame)
find_path(LAME_INCLUDE_DIR NAMES lame/lame.h)
if(LAME_LIBRARY AND LAME_INCLUDE_DIR)
    add_library(LAME::lame UNKNOWN IMPORTED)
    set_target_properties(LAME::lame PROPERTIES
        IMPORTED_LOCATION "${LAME_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LAME_INCLUDE_DIR}"
    )
    set(LAME_FOUND TRUE)
endif()

pkg_check_modules(OGG IMPORTED_TARGET ogg)
pkg_check_modules(TWOLAME IMPORTED_TARGET twolame)
pkg_check_modules(OPUS IMPORTED_TARGET opus)
pkg_check_modules(VORBIS IMPORTED_TARGET vorbis vorbisenc)
pkg_check_modules(FLAC IMPORTED_TARGET flac)
pkg_check_modules(FDK_AAC IMPORTED_TARGET fdk-aac)
pkg_check_modules(SAMPLERATE IMPORTED_TARGET samplerate)

# Streaming
find_library(SHOUT_LIBRARY NAMES shout)
find_path(SHOUT_INCLUDE_DIR NAMES shout/shout.h)
if(SHOUT_LIBRARY AND SHOUT_INCLUDE_DIR)
    add_library(SHOUT::shout UNKNOWN IMPORTED)
    set_target_properties(SHOUT::shout PROPERTIES
        IMPORTED_LOCATION "${SHOUT_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${SHOUT_INCLUDE_DIR}"
    )
    set(SHOUT_FOUND TRUE)
endif()

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

        # Config
        src/config/ConfigManager.cpp
        src/config/ConfigManager.h

        # Dialogs
        src/ui/dialogs/SettingsDialog.cpp
        src/ui/dialogs/SettingsDialog.h
        src/ui/dialogs/AudioFxDialog.cpp
        src/ui/dialogs/AudioFxDialog.h
        src/ui/dialogs/MixerDialog.cpp
        src/ui/dialogs/MixerDialog.h
        src/ui/dialogs/SchedulerDialog.cpp
        src/ui/dialogs/SchedulerDialog.h
        src/ui/dialogs/ManagerDialogs.cpp
        src/ui/dialogs/ManagerDialogs.h

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
        src/ui/ConsoleLogComponent.cpp
        src/ui/ConsoleLogComponent.h
        src/ui/Icons.h
    )

    target_include_directories(OpenRadioEncoder PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/libs/butt-core
    )

    target_compile_definitions(OpenRadioEncoder PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_PLUGINHOST_VST3=1
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:OpenRadioEncoder,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:OpenRadioEncoder,JUCE_VERSION>"
        JUCE_MODAL_LOOPS_PERMITTED=1
    )

    juce_generate_juce_header(OpenRadioEncoder)

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
    # ─────────────────────────────────────────────
    # Installation & Packaging
    # ─────────────────────────────────────────────
    include(GNUInstallDirs)

    if(WIN32)
        install(TARGETS OpenRadioEncoder
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )

        install(TARGETS OpenRadioEncoder
            RUNTIME_DEPENDENCIES
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
            POST_EXCLUDE_REGEXES ".*system32.*" ".*\\.pdb$"
        )

        install(FILES LICENSE DESTINATION . OPTIONAL)

        set(CPACK_GENERATOR "NSIS")
        set(CPACK_PACKAGE_FILE_NAME "OpenRadioEncoder-${PROJECT_VERSION}-win64")
        set(CPACK_NSIS_DISPLAY_NAME "Open Radio Encoder")
        set(CPACK_NSIS_PACKAGE_NAME "Open Radio Encoder ${PROJECT_VERSION}")
        set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
        set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/appicon.png")
        set(CPACK_NSIS_MUI_UNICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/appicon.png")
        set(CPACK_NSIS_CREATE_ICONS_EXTRA "
            CreateShortCut '$SMPROGRAMS\\\\$STARTMENU_FOLDER\\\\Open Radio Encoder.lnk' '$INSTDIR\\\\bin\\\\OpenRadioEncoder.exe'
            CreateShortCut '$DESKTOP\\\\Open Radio Encoder.lnk' '$INSTDIR\\\\bin\\\\OpenRadioEncoder.exe'
        ")
        set(CPACK_NSIS_DELETE_ICONS_EXTRA "
            Delete '$SMPROGRAMS\\\\$START_MENU\\\\Open Radio Encoder.lnk'
            Delete '$DESKTOP\\\\Open Radio Encoder.lnk'
        ")
        set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/danielrodrigohub/Open-Radio-Encoder")
        set(CPACK_NSIS_HELP_LINK "https://github.com/danielrodrigohub/Open-Radio-Encoder/issues")
        set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
        set(CPACK_NSIS_MODIFY_PATH OFF)
        set(CPACK_NSIS_MENU_LINKS
            "https://github.com/danielrodrigohub/Open-Radio-Encoder" "Open Radio Encoder on GitHub"
        )
    elseif(APPLE)
        install(TARGETS OpenRadioEncoder BUNDLE DESTINATION .)
        set(CPACK_GENERATOR "DragNDrop")
        set(CPACK_DMG_VOLUME_NAME "Open Radio Encoder")
    else()
        install(TARGETS OpenRadioEncoder RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
        set(CPACK_GENERATOR "TGZ")
    endif()

    set(CPACK_PACKAGE_NAME "OpenRadioEncoder")
    set(CPACK_PACKAGE_VENDOR "OpenRadioProject")
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Multi-station internet radio streaming encoder")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/danielrodrigohub/Open-Radio-Encoder")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY "Open Radio Encoder")

    include(CPack)
else()
    # Fallback: Build without JUCE for CI/testing
    message(WARNING "Building without JUCE - engine library only")
endif()
