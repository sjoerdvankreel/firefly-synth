cmake_minimum_required(VERSION 3.21)
project(infernal-synth)

set(INFERNAL_VERSION "2.0")
set(INFERNAL_NAME "infernal.synth")
set(INFERNAL_FX_NAME "infernal.synth.fx")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(infernal.vst3)
include(infernal.config)
include(infernal.utility)

add_subdirectory(base)

declare_local_target(STATIC infernal.synth infernal.synth)
target_include_directories(infernal.synth PRIVATE base/src/infernal.base)
declare_vst3_target(${INFERNAL_NAME} infernal.synth.vst3 infernal.synth 0)
declare_vst3_target(${INFERNAL_FX_NAME} infernal.synth.vst3 infernal.synth 1)
declare_clap_target(${INFERNAL_NAME} infernal.synth.clap infernal.synth 0)
declare_clap_target(${INFERNAL_FX_NAME} infernal.synth.clap infernal.synth 1)