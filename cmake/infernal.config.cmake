cmake_policy(SET CMP0091 NEW)

if(CMAKE_HOST_WIN32)
add_compile_options(/W4;/WX;/external:W0;/wd4245;/wd4267;/wd4244;/wd4100;/wd26495;/fp:fast;/arch:AVX)
endif()
if(CMAKE_HOST_LINUX)
add_compile_options(-Wall;-Wextra;-Werror;-Wno-cpp;-Wno-sign-compare;-Wno-unused-parameter;-Wfatal-errors;-ffast-math;-march=avx)
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")