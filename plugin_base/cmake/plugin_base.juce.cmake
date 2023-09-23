add_definitions(-DJUCE_USE_CURL=0)
add_definitions(-DJUCE_DISPLAY_SPLASH_SCREEN=0)
add_definitions(-DJUCE_MODAL_LOOPS_PERMITTED=0)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
add_definitions(-DJUCE_WINDOWS=1)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
add_definitions(-DJUCE_LINUX=1)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DJUCE_DEBUG=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DJUCE_DEBUG=0")
endif()

set(JUCE_MODULES_ONLY ON)
set(JUCE_ENABLE_MODULE_SOURCE_GROUPS ON)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/JUCE)