cmake_minimum_required(VERSION 3.22)

project(AudioPlugin02 VERSION 0.3.0)

# ============================================================
# C++ Standard
# ============================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# JUCE - FetchContent
# ============================================================

include(FetchContent)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

# ============================================================
# Plugin Definition
# ============================================================

juce_add_plugin(AudioPlugin02
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Ycmp
    PLUGIN_CODE Ap02
    FORMATS Standalone VST3 AU
    PRODUCT_NAME "plugin-camera-v1"
    PLIST_TO_MERGE "<plist><dict><key>NSCameraUsageDescription</key><string>This plugin displays the camera feed for visual feedback.</string></dict></plist>"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    NEEDS_JUCE_CORE TRUE
    NEEDS_JUCE_AUDIO_PROCESSORS TRUE
    NEEDS_JUCE_GUI_BASICS TRUE
    NEEDS_WEB_BROWSER TRUE

    COPY_PLUGIN_AFTER_BUILD TRUE

    VST3_CATEGORIES Fx Reverb
    AU_MAIN_TYPE kAudioUnitType_Effect
)

juce_generate_juce_header(AudioPlugin02)

# ============================================================
# Source Files
# ============================================================

target_sources(AudioPlugin02 PRIVATE
    # Plugin core
    Source/Plugin/PluginEditor.cpp
    Source/Plugin/PluginEditor.h
    Source/Plugin/PluginProcessor.cpp
    Source/Plugin/PluginProcessor.h
    Source/Plugin/WebviewBridge.h

    # Parameters
    Source/Parameters/ParameterIDs.h
    Source/Parameters/ParameterLayout.h

    # DSP - Header-only utilities
    Source/DSP/DiffusionNetwork/SmoothedValue.h
    Source/DSP/DiffusionNetwork/LFO.h
    Source/DSP/DiffusionNetwork/DelayLine.h
    Source/DSP/DiffusionNetwork/FeedbackMatrix.h
    Source/DSP/DiffusionNetwork/DiffusionStage.h
    Source/DSP/Decay/Freeze.h
    Source/DSP/Output/StereoWidener.h
    Source/DSP/Output/DryWetMixer.h

    # DSP - Input modules
    Source/DSP/Input/GainModule.h

    # DSP - Filters
    Source/DSP/Input/InputFilter.h
    Source/DSP/Input/InputFilter.cpp
    Source/DSP/DiffusionNetwork/CrossoverFilter.h
    Source/DSP/DiffusionNetwork/CrossoverFilter.cpp

    # DSP - Delay-based
    Source/DSP/Input/Predelay.h
    Source/DSP/Input/Predelay.cpp
    Source/DSP/EarlyReflections/SpinModulator.h
    Source/DSP/EarlyReflections/SpinModulator.cpp

    # DSP - Early Reflections
    Source/DSP/EarlyReflections/EarlyReflections.h
    Source/DSP/EarlyReflections/EarlyReflections.cpp

    # DSP - Diffusion
    Source/DSP/DiffusionNetwork/DiffusionNetwork.h
    Source/DSP/DiffusionNetwork/DiffusionNetwork.cpp

    # DSP - FDN Core
    Source/DSP/Decay/FDNReverb.h
    Source/DSP/Decay/FDNReverb.cpp

    # DSP - Modulation (input conditioning + shimmer section)
    Source/DSP/Modulation/Saturation.h
    Source/DSP/Modulation/Saturation.cpp
    Source/DSP/Modulation/TimbreFilter.h
    Source/DSP/Modulation/TimbreFilter.cpp
    Source/DSP/Modulation/Vibrato.h
    Source/DSP/Modulation/Vibrato.cpp
    Source/DSP/Modulation/PitchShifter.h
    Source/DSP/Modulation/PitchShifter.cpp
    Source/DSP/DiffusionNetwork/Chorus.h
    Source/DSP/DiffusionNetwork/Chorus.cpp
)

if(APPLE)
    # Compile PluginEditor.cpp as Objective-C++ so it can use WebKit/AVFoundation directly
    set_source_files_properties(Source/Plugin/PluginEditor.cpp
        PROPERTIES COMPILE_FLAGS "-x objective-c++")
endif()

target_include_directories(AudioPlugin02 PRIVATE
    Source/Plugin
    Source/DSP
    Source/Parameters
)

# ============================================================
# Svelte Build Logic
# ============================================================

file(MAKE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/WebUI/dist")

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/WebUI/dist/index.html")
    file(WRITE "${CMAKE_CURRENT_SOURCE_DIR}/WebUI/dist/index.html" "<html><body>Loading...</body></html>")
endif()

add_custom_target(BuildSvelteUI ALL
    COMMAND npm install
    COMMAND npm run build
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/WebUI"
    COMMENT "Building Svelte frontend..."
    VERBATIM
)

# ============================================================
# Embed Web Resources Into Binary
# ============================================================

juce_add_binary_data(AudioPlugin02_Resources
    SOURCES
        WebUI/dist/index.html
)

add_dependencies(AudioPlugin02_Resources BuildSvelteUI)

# ============================================================
# Compile Definitions & Linking
# ============================================================

target_compile_definitions(AudioPlugin02 PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

if(APPLE)
    target_compile_options(AudioPlugin02 PRIVATE
        -Wno-deprecated-declarations
    )
endif()

if(APPLE)
    target_link_libraries(AudioPlugin02 PRIVATE
        "-framework AVFoundation"
        "-framework WebKit"
    )
endif()

target_link_libraries(AudioPlugin02 PRIVATE
    AudioPlugin02_Resources
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_gui_extra
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

# ============================================================
# Copy Resources to Bundle (Post-Build)
# ============================================================

macro(copy_resources_to_target target_name)
    if(TARGET ${target_name})
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/WebUI/dist"
                "$<TARGET_FILE_DIR:${target_name}>/../Resources"
        )
    endif()
endmacro()

copy_resources_to_target(AudioPlugin02)
copy_resources_to_target(AudioPlugin02_Standalone)
