cmake_policy(SET CMP0091 NEW)
cmake_minimum_required(VERSION 3.21)
project(infernal-synth)

set(INFERNAL_VERSION "2.0")
set(INFERNAL_NAME "infernal.synth")
set(INFERNAL_FX_NAME "infernal.synth.fx")

# Config
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

add_compile_options(/W4;/WX;/external:W0;/wd4245;/wd4267;/wd4244;/wd4100;/wd26495;/fp:fast;/arch:AVX)

# Utility
function(declare_local_target LIBRARY_TYPE PROJECT_NAME SOURCE_DIR)
  set("${PROJECT_NAME}_SRC_DIR" "src/${SOURCE_DIR}")
  file(GLOB_RECURSE "${PROJECT_NAME}_SRC" "${${PROJECT_NAME}_SRC_DIR}/*.*")
  add_library(${PROJECT_NAME} ${LIBRARY_TYPE} "${${PROJECT_NAME}_SRC}")
  target_include_directories(${PROJECT_NAME} PRIVATE "${${PROJECT_NAME}_SRC_DIR}")
  source_group(TREE "../../${${PROJECT_NAME}_SRC_DIR}" FILES "${${PROJECT_NAME}_SRC}")
endfunction()

function(declare_vst3_target VST3_PROJECT_NAME PLUG_PROJECT_NAME)
  set(TARGET_NAME "${VST3_PROJECT_NAME}.${INFERNAL_VERSION}.vst3")
  declare_local_target(SHARED ${TARGET_NAME} ${PLUG_PROJECT_NAME})
  target_include_directories(${TARGET_NAME} SYSTEM PRIVATE lib/vst3)
  target_include_directories(${TARGET_NAME} PRIVATE "src/${PLUG_PROJECT_NAME}" src/infernal.base src/infernal.base.vst3)
  target_link_libraries(${TARGET_NAME} infernal.base infernal.base.vst3 ${PLUG_PROJECT_NAME} pluginterfaces sdk sdk_common)
endfunction()

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