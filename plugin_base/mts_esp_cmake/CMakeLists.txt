cmake_minimum_required(VERSION 3.21)
project(plugin_base.mts_esp)
add_library(plugin_base.mts_esp STATIC ${CMAKE_SOURCE_DIR}/lib/MTS-ESP/Client/libMTSClient.cpp ${CMAKE_SOURCE_DIR}/lib/MTS-ESP/Client/libMTSClient.h)
target_compile_definitions(plugin_base.mts_esp PRIVATE _CRT_SECURE_NO_WARNINGS=1)