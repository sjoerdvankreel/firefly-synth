cmake_minimum_required(VERSION 3.25)
project(infernal-base)

declare_local_target(STATIC infernal.base infernal.base)
declare_local_target(STATIC infernal.base.clap infernal.base.clap)
declare_local_target(STATIC infernal.base.vst3 infernal.base.vst3)
target_include_directories(infernal.base.vst3 PRIVATE src/infernal.base)
target_include_directories(infernal.base.vst3 SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/lib/vst3)