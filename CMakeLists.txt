cmake_policy(SET CMP0091 NEW)
cmake_minimum_required(VERSION 3.21)
project(infernal-synth)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

add_compile_options(/W4;/WX;/external:W0;/wd4267;/wd4244;/wd4100;/wd26495;/fp:fast;/arch:AVX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")
include(SMTG_VST3_SDK)
smtg_setup_platform_toolset()
add_subdirectory(lib/vst3/base)
add_subdirectory(lib/vst3/public.sdk)
add_subdirectory(lib/vst3/pluginterfaces)

function(declare_local_target LIBRARY_TYPE PROJECT_NAME SOURCE_DIR)
  set("${PROJECT_NAME}_SRC_DIR" "src/${SOURCE_DIR}")
  file(GLOB_RECURSE "${PROJECT_NAME}_SRC" "${${PROJECT_NAME}_SRC_DIR}/*.*")
  add_library(${PROJECT_NAME} ${LIBRARY_TYPE} "${${PROJECT_NAME}_SRC}")
  target_include_directories(${PROJECT_NAME} PRIVATE "${${PROJECT_NAME}_SRC_DIR}")
  source_group(TREE "../../${${PROJECT_NAME}_SRC_DIR}" FILES "${${PROJECT_NAME}_SRC}")
endfunction()

declare_local_target(STATIC infernal.base infernal.base)
declare_local_target(STATIC infernal.base.clap infernal.base.clap)
declare_local_target(STATIC infernal.base.vst3 infernal.base.vst3)

declare_local_target(STATIC infernal.synth infernal.synth)
declare_local_target(SHARED infernal.synth.clap infernal.synth.clap)
declare_local_target(SHARED infernal.synth.vst3 infernal.synth.vst3)
declare_local_target(SHARED infernal.synth.vst3i infernal.synth.vst3)

target_include_directories(infernal.synth PRIVATE src/infernal.base)

target_include_directories(infernal.base.vst3 SYSTEM PRIVATE lib/vst3)
target_include_directories(infernal.base.vst3 PRIVATE src/infernal.base)

target_compile_definitions(infernal.synth.vst3 PRIVATE INF_SYNTH_FX=1)
target_include_directories(infernal.synth.vst3 SYSTEM PRIVATE lib/vst3)
target_include_directories(infernal.synth.vst3 PRIVATE src/infernal.synth)
target_include_directories(infernal.synth.vst3 PRIVATE src/infernal.base)
target_include_directories(infernal.synth.vst3 PRIVATE src/infernal.base.vst3)

target_compile_definitions(infernal.synth.vst3i PRIVATE INF_SYNTH_FX=0)
target_include_directories(infernal.synth.vst3i SYSTEM PRIVATE lib/vst3)
target_include_directories(infernal.synth.vst3i PRIVATE src/infernal.synth)
target_include_directories(infernal.synth.vst3i PRIVATE src/infernal.base)
target_include_directories(infernal.synth.vst3i PRIVATE src/infernal.base.vst3)

