cmake_minimum_required(VERSION 3.22)
project(PsyForge VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS version")

# JUCE: lokaler Ordner ./JUCE (falls vorhanden), sonst automatisch von GitHub
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(juce
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW ON)
    FetchContent_MakeAvailable(juce)
endif()

set(PSYFORGE_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND PSYFORGE_FORMATS AU)
endif()

juce_add_plugin(PsyForge
    COMPANY_NAME "JustCreateit"
    COMPANY_WEBSITE "https://justcreateit.ch"
    BUNDLE_ID "ch.justcreateit.psyforge"
    PLUGIN_MANUFACTURER_CODE Jcit
    PLUGIN_CODE Psfg
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    PRODUCT_NAME "PsyForge"
    FORMATS ${PSYFORGE_FORMATS})

target_sources(PsyForge PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(PsyForge PRIVATE Source)
# Quelltexte sind UTF-8 (Umlaute in Hilfetexten)
target_compile_options(PsyForge PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_compile_definitions(PsyForge PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(PsyForge
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_formats
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Offline-Test ohne DAW: rendert alle Presets als WAV und prüft die Analyse
option(PSYFORGE_BUILD_TEST "Build offline render/analysis test" OFF)
if(PSYFORGE_BUILD_TEST)
    add_executable(psyforge_test Tools/RenderTest.cpp)
    target_include_directories(psyforge_test PRIVATE Source)
endif()

# Integrationstest mit JUCE (Prozessor, Export, Analyse, Bildschirmfotos der Oberfläche)
if(PSYFORGE_BUILD_TEST)
    juce_add_gui_app(PsyForgeUiTest PRODUCT_NAME "PsyForgeUiTest")
    target_sources(PsyForgeUiTest PRIVATE Tools/UiTest.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_include_directories(PsyForgeUiTest PRIVATE Source)
    target_compile_definitions(PsyForgeUiTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1
        JucePlugin_Name="PsyForge" JucePlugin_IsSynth=1 JucePlugin_WantsMidiInput=1 JucePlugin_ProducesMidiOutput=0 JucePlugin_IsMidiEffect=0)
    target_link_libraries(PsyForgeUiTest PRIVATE juce::juce_audio_utils juce::juce_audio_formats juce::juce_dsp juce::juce_recommended_config_flags)
endif()
