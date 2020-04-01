cmake_minimum_required (VERSION 3.5)
project(sfizz-cpuid)

add_library(sfizz-cpuid STATIC src/cpuid/cpuinfo.cpp src/cpuid/version.cpp)
set_property(TARGET sfizz-cpuid PROPERTY CXX_STANDARD 11)
target_include_directories(sfizz-cpuid PUBLIC src PRIVATE platform/src)
