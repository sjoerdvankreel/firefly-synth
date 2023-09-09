cmake_minimum_required(VERSION 3.21)
project(plugin_base)

declare_local_target(STATIC plugin_base plugin_base)
declare_local_target(STATIC plugin_base.juce plugin_base.juce)
declare_local_target(STATIC plugin_base.clap plugin_base.clap)
declare_local_target(STATIC plugin_base.vst3 plugin_base.vst3)

target_link_libraries(plugin_base.juce juce_core juce_events juce_data_structures juce_graphics juce_gui_basics)

target_include_directories(plugin_base SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/lib/JUCE/modules)
target_include_directories(plugin_base.vst3 PRIVATE src/plugin_base)
target_include_directories(plugin_base.vst3 SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/lib/vst3)
target_include_directories(plugin_base.clap PRIVATE src/plugin_base)
target_include_directories(plugin_base.clap SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/lib/clap/include ${CMAKE_SOURCE_DIR}/lib/clap-helpers/include)

# disable unreachable code warnings for clap-helper headers
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
target_compile_options(plugin_base.clap PRIVATE /wd4702)
endif()
