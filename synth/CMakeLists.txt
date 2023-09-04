cmake_minimum_required(VERSION 3.21)
project(infernal-synth)

set(INFERNAL_VERSION "2.0")
set(INFERNAL_NAME "infernal.synth")
set(INFERNAL_FX_NAME "infernal.synth.fx")

declare_local_target(STATIC infernal.synth infernal.synth)
declare_local_target(SHARED infernal.synth.clap infernal.synth.clap)
declare_vst3_target(${INFERNAL_NAME} infernal.synth 0)
declare_vst3_target(${INFERNAL_FX_NAME} infernal.synth 1)
target_include_directories(infernal.synth PRIVATE "${CMAKE_SOURCE_DIR}/base/src/infernal.base")