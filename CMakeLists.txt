cmake_minimum_required(VERSION 3.21)
project(infernal-synth)

include(cmake/infernal.config.cmake)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(infernal.utility)

set(INFERNAL_VERSION "2.0")
set(INFERNAL_NAME "infernal.synth")
set(INFERNAL_FX_NAME "infernal.synth.fx")

# VST3 SDK
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/vst3/cmake/modules")
include(SMTG_VST3_SDK)
smtg_setup_platform_toolset()

set(VST_SDK TRUE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)

add_subdirectory(lib/vst3/base)
add_subdirectory(lib/vst3/public.sdk)
add_subdirectory(lib/vst3/pluginterfaces)

# Base libraries
declare_local_target(STATIC infernal.base infernal.base)
declare_local_target(STATIC infernal.base.clap infernal.base.clap)
declare_local_target(STATIC infernal.base.vst3 infernal.base.vst3)
target_include_directories(infernal.base.vst3 SYSTEM PRIVATE lib/vst3)
target_include_directories(infernal.base.vst3 PRIVATE src/infernal.base)

# Plugin libraries
declare_local_target(STATIC infernal.synth infernal.synth)
declare_local_target(SHARED infernal.synth.clap infernal.synth.clap)
target_include_directories(infernal.synth PRIVATE src/infernal.base)
declare_vst3_target(${INFERNAL_NAME} infernal.synth)
declare_vst3_target(${INFERNAL_FX_NAME} infernal.synth)