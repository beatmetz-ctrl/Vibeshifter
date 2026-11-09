cmake_minimum_required(VERSION 3.15)
project(Deal8 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Deal8
    COMPANY_NAME "MeatTheBeat"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE MtBt
    PLUGIN_CODE Del8
    FORMATS VST3 Standalone
    PRODUCT_NAME "Deal8"
)

juce_generate_juce_header(Deal8)

target_sources(Deal8
    PRIVATE
        PitchFormantShifter.h
        PluginProcessor.h
        PluginProcessor.cpp
        PluginEditor.h
        PluginEditor.cpp
)

target_compile_definitions(Deal8
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(Deal8
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
