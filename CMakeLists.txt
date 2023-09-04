cmake_minimum_required(VERSION 3.21)
project(infernal-synth)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(infernal.config)
include(infernal.vst3)
include(infernal.utility)

set(INFERNAL_VERSION "2.0")
set(INFERNAL_NAME "infernal.synth")
set(INFERNAL_FX_NAME "infernal.synth.fx")

add_subdirectory(base)

# Plugin libraries
declare_local_target(STATIC infernal.synth infernal.synth)
declare_local_target(SHARED infernal.synth.clap infernal.synth.clap)
target_include_directories(infernal.synth PRIVATE base/src/infernal.base)
declare_vst3_target(${INFERNAL_NAME} infernal.synth)
declare_vst3_target(${INFERNAL_FX_NAME} infernal.synth)