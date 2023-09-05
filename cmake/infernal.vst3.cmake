list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/lib/vst3/cmake/modules")
include(SMTG_VST3_SDK)
smtg_setup_platform_toolset()

set(VST_SDK TRUE)
set(SMTG_ENABLE_VSTGUI_SUPPORT OFF)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEVELOPMENT=1")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DRELEASE=1")

add_subdirectory(${CMAKE_SOURCE_DIR}/lib/vst3/base)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/vst3/public.sdk)
add_subdirectory(${CMAKE_SOURCE_DIR}/lib/vst3/pluginterfaces)