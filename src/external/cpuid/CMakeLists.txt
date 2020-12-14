cmake_minimum_required (VERSION 3.5)
project(sfizz_cpuid)

add_library(sfizz_cpuid STATIC src/cpuid/cpuinfo.cpp src/cpuid/version.cpp)
add_library(sfizz::cpuid ALIAS sfizz_cpuid)
set_property(TARGET sfizz_cpuid PROPERTY CXX_STANDARD 11)
target_include_directories(sfizz_cpuid PUBLIC src PRIVATE platform/src)
