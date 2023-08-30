cmake_policy(SET CMP0091 NEW)
cmake_minimum_required(VERSION 3.21)
project(infernal-synth)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")

function(declare_local_target PROJECT_NAME)
  set("${PROJECT_NAME}_SRC_DIR" "src/${PROJECT_NAME}")
  file(GLOB_RECURSE "${PROJECT_NAME}_SRC" "${${PROJECT_NAME}_SRC_DIR}/*.*")
  add_library(${PROJECT_NAME} STATIC "${${PROJECT_NAME}_SRC}")
  target_include_directories(${PROJECT_NAME} PRIVATE "${${PROJECT_NAME}_SRC_DIR}")
  if (MSVC)
     source_group(TREE "../../${${PROJECT_NAME}_SRC_DIR}" FILES "${${PROJECT_NAME}_SRC}")
  endif()
endfunction()

declare_local_target(infernal.base)
declare_local_target(infernal.base.clap)

declare_local_target(infernal.base.vst3)
target_include_directories(infernal.base.vst3 PRIVATE src/infernal.base)
target_include_directories(infernal.base.vst3 SYSTEM PRIVATE lib lib/pluginterfaces lib/vst3_public_sdk/source)

declare_local_target(infernal.synth)
declare_local_target(infernal.synth.clap)
declare_local_target(infernal.synth.vst3)
