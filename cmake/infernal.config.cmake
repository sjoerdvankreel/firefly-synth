cmake_policy(SET CMP0091 NEW)
add_compile_options(/W4;/WX;/external:W0;/wd4245;/wd4267;/wd4244;/wd4100;/wd26495;/fp:fast;/arch:AVX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;Release")