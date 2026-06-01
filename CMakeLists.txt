cmake_minimum_required(VERSION 3.22)
project(SpatialPanner VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(SpatialPanner
    COMPANY_NAME                "SpatialTools"
    PLUGIN_MANUFACTURER_CODE    Spat
    PLUGIN_CODE                 Span
    FORMATS                     AU VST3 Standalone
    PRODUCT_NAME                "Spatial Panner"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             "Fx" "Spatial"
    AU_MAIN_TYPE                kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

target_sources(SpatialPanner PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/MultiCanvas.cpp
    Source/GlobalSpatialRegistry.cpp
)

target_include_directories(SpatialPanner PRIVATE Source)

target_compile_definitions(SpatialPanner PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

juce_generate_juce_header(SpatialPanner)

target_link_libraries(SpatialPanner PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
