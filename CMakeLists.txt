cmake_minimum_required(VERSION 3.22)
project(WaveBombzPUNCH VERSION 0.1.0 LANGUAGES CXX)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(WaveBombzPUNCH COMPANY_NAME "WaveBombz" BUNDLE_ID "com.wavebombz.punch"
 PLUGIN_MANUFACTURER_CODE WvBz PLUGIN_CODE Wpnc PRODUCT_NAME "WaveBombz PUNCH"
 DESCRIPTION "Transient shaper and punch enhancer" IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE
 NEEDS_MIDI_OUTPUT FALSE IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
 COPY_PLUGIN_AFTER_BUILD FALSE FORMATS VST3 VST3_CATEGORIES Fx Dynamics)
juce_generate_juce_header(WaveBombzPUNCH)

target_sources(WaveBombzPUNCH PRIVATE Source/PluginProcessor.cpp Source/PluginProcessor.h Source/PluginEditor.cpp Source/PluginEditor.h)
target_compile_features(WaveBombzPUNCH PRIVATE cxx_std_17)
target_compile_definitions(WaveBombzPUNCH PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(WaveBombzPUNCH PRIVATE juce::juce_audio_utils juce::juce_dsp
 PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
