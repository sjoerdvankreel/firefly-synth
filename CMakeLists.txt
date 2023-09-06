cmake_minimum_required(VERSION 3.21)
project(infernal_synth)

set(INFERNAL_VERSION "2.0")
set(INFERNAL_NAME "infernal_synth")
set(INFERNAL_FX_NAME "infernal_synth.fx")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/plugin_base/cmake")
include(plugin_base.vst3)
include(plugin_base.config)
include(plugin_base.utility)

add_subdirectory(plugin_base)

declare_local_target(STATIC infernal_synth infernal_synth)
target_include_directories(infernal_synth PRIVATE plugin_base/src/plugin_base)
declare_vst3_target(${INFERNAL_NAME} infernal_synth.vst3 infernal_synth 0)
declare_vst3_target(${INFERNAL_FX_NAME} infernal_synth.vst3 infernal_synth 1)
declare_clap_target(${INFERNAL_NAME} infernal_synth.clap infernal_synth 0)
declare_clap_target(${INFERNAL_FX_NAME} infernal_synth.clap infernal_synth 1)