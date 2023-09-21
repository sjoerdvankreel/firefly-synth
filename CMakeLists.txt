cmake_minimum_required(VERSION 3.21)
project(infernal_synth)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)
add_compile_definitions(JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)

set(PLUGIN_VERSION "2")
set(PLUGIN_NAME "infernal_synth")
set(PLUGIN_FX_NAME "infernal_synth_fx")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/plugin_base/cmake")
include(plugin_base.vst3)
include(plugin_base.juce)
include(plugin_base.config)
include(plugin_base.utility)

add_subdirectory(plugin_base)
declare_local_target(STATIC infernal_synth infernal_synth)
target_include_directories(infernal_synth PRIVATE plugin_base/src/plugin_base)

declare_vst3_target(${PLUGIN_NAME} infernal_synth.vst3 infernal_synth 0)
declare_vst3_target(${PLUGIN_FX_NAME} infernal_synth.vst3 infernal_synth 1)
declare_clap_target(${PLUGIN_NAME} infernal_synth.clap infernal_synth 0)
declare_clap_target(${PLUGIN_FX_NAME} infernal_synth.clap infernal_synth 1)