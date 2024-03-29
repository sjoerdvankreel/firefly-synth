cmake_minimum_required(VERSION 3.21)
project(plugin_base)

declare_local_target(FALSE STATIC plugin_base plugin_base)
declare_local_target(FALSE STATIC plugin_base.juce plugin_base.juce)
declare_local_target(FALSE STATIC plugin_base.clap plugin_base.clap)
declare_local_target(FALSE STATIC plugin_base.vst3 plugin_base.vst3)
declare_local_target(TRUE STATIC plugin_base.ref_gen plugin_base.ref_gen)

target_include_directories(plugin_base SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/lib/JUCE/modules)
target_include_directories(plugin_base.vst3 PRIVATE src/plugin_base)
target_include_directories(plugin_base.vst3 SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/lib/vst3 ${CMAKE_SOURCE_DIR}/lib/JUCE/modules)
target_include_directories(plugin_base.clap PRIVATE src/plugin_base)
target_include_directories(plugin_base.clap SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/lib/clap/include ${CMAKE_SOURCE_DIR}/lib/clap-helpers/include ${CMAKE_SOURCE_DIR}/lib/JUCE/modules ${CMAKE_SOURCE_DIR}/lib/readerwriterqueue)
target_link_libraries(plugin_base.juce PRIVATE juce_core juce_events juce_data_structures juce_cryptography juce_graphics juce_gui_basics juce_dsp)

target_include_directories(plugin_base.ref_gen PRIVATE src/plugin_base)
target_include_directories(plugin_base.ref_gen SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/lib/JUCE/modules)
target_link_libraries(plugin_base.ref_gen plugin_base plugin_base plugin_base.juce)
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(BUILD_TYPE "$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>$<$<CONFIG:RelWithDebInfo>:RelWithDebInfo>")
  set_target_properties(plugin_base.ref_gen PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/${BUILD_TYPE}/win")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set_target_properties(plugin_base.ref_gen PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/${CMAKE_BUILD_TYPE}/linux")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  set_target_properties(plugin_base.ref_gen PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/dist/${CMAKE_BUILD_TYPE}/mac")
else()
  message(FATAL_ERROR)
endif()

# disable unreachable code warnings for clap-helper headers
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
target_compile_options(plugin_base.clap PRIVATE /wd4702)
endif()

# disable gcc warnings for juce
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
target_compile_options(plugin_base.juce PRIVATE -Wno-suggest-override)
endif()