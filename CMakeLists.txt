cmake_minimum_required(VERSION 3.22)
project(SoundLighter VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory($ENV{HOME}/Dev/JUCE JUCE)

juce_add_gui_app(SpectrumToColour
    PRODUCT_NAME "Sound Lighter"
    BUNDLE_ID    "com.yourname.soundlighter"
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Sound Lighter needs microphone access to analyse audio"
)

juce_generate_juce_header(SpectrumToColour)

target_sources(SpectrumToColour PRIVATE
    src/Main.cpp
    src/MainComponent.h
    src/MainComponent.cpp
    src/FrequencyToColour.h
    src/SpectrumAnalyser.h
    src/SpectrumAnalyser.cpp
)

target_compile_definitions(SpectrumToColour PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:SpectrumToColour,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:SpectrumToColour,JUCE_VERSION>"
)

target_link_libraries(SpectrumToColour PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_dsp
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)
