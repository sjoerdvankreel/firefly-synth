set(JUCE_MODULES_ONLY ON)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/JUCE)
juce_add_module(${CMAKE_SOURCE_DIR}/lib/JUCE/modules/juce_core)