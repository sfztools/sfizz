if (WIN32)
    cmake_minimum_required (VERSION 3.15)
    cmake_policy(SET CMP0091 NEW)
else()
    cmake_minimum_required (VERSION 3.5)
    if (POLICY CMP0069)
        cmake_policy(SET CMP0069 NEW)
    endif()
endif()

project (sfizz VERSION 0.3.1 LANGUAGES CXX C)
set (PROJECT_DESCRIPTION "A library to load SFZ description files and use them to render music.")

# External configuration CMake scripts
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (BuildType)
include (SfizzConfig)

# Build Options
set (BUILD_TESTING OFF CACHE BOOL "Disable Abseil's tests [default: OFF]")

option (ENABLE_LTO              "Enable Link Time Optimization [default: ON]" ON)
option (SFIZZ_JACK              "Enable JACK stand-alone build [default: ON]" ON)
option (SFIZZ_RENDER            "Enable renderer of SMF files [default: ON]" ON)
option (SFIZZ_LV2               "Enable LV2 plug-in build [default: ON]" ON)
option (SFIZZ_VST               "Enable VST plug-in build [default: OFF]" OFF)
option (SFIZZ_AU                "Enable AU plug-in build [default: OFF]" OFF)
option (SFIZZ_BENCHMARKS        "Enable benchmarks build [default: OFF]" OFF)
option (SFIZZ_TESTS             "Enable tests build [default: OFF]" OFF)
option (SFIZZ_DEVTOOLS          "Enable developer tools build [default: OFF]" OFF)
option (SFIZZ_SHARED            "Enable shared library build [default: ON]" ON)
option (SFIZZ_USE_VCPKG         "Assume that sfizz is build using vcpkg [default: OFF]" OFF)
option (SFIZZ_STATIC_LIBSNDFILE "Link libsndfile statically [default: OFF]" OFF)
option (SFIZZ_RELEASE_ASSERTS   "Forced assertions in release builds [default: OFF]" OFF)

# Don't use IPO in non Release builds
include (CheckIPO)

# Add Abseil
add_subdirectory (external/abseil-cpp EXCLUDE_FROM_ALL)

# Add the static library targets and sources
add_subdirectory (src)

# Optional targets
add_subdirectory (clients)

if (SFIZZ_LV2 OR SFIZZ_VST)
    add_subdirectory (editor)
endif()

if (SFIZZ_LV2)
    add_subdirectory (lv2)
endif()

if (SFIZZ_VST)
    add_subdirectory (vst)
else()
    if (SFIZZ_AU)
        message(WARNING "Audio Unit requires VST to be enabled")
    endif()
endif()

if (SFIZZ_BENCHMARKS)
    add_subdirectory (benchmarks)
endif()

if (SFIZZ_TESTS)
    add_subdirectory (tests)
endif()

if (SFIZZ_DEVTOOLS)
    add_subdirectory (devtools)
endif()

# Put it at the end so that the vst/lv2 directories are registered
if (NOT MSVC)
    include(SfizzUninstall)
endif()

show_build_info_if_needed()
