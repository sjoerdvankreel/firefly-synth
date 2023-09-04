cmake_minimum_required(VERSION 3.21)
project(infernal-build)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(infernal.vst3)
include(infernal.config)
include(infernal.utility)

add_subdirectory(base)
add_subdirectory(synth)