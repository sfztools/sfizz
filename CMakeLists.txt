if(WIN32)
    cmake_minimum_required(VERSION 3.15)
    cmake_policy(SET CMP0091 NEW)
else()
    # USE_LIBCPP requires add_link_options() in SfizzConfig.cmake
    cmake_minimum_required(VERSION 3.13)
endif()

project(libsfizz
    LANGUAGES CXX C
    VERSION 1.2.2
)
set(PROJECT_DESCRIPTION "A library to load SFZ description files and use them to render music.")
set(PROJECT_REPOSITORY https://github.com/sfztools/sfizz)

# External configuration CMake scripts
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(BuildType)

# Build Options
include(OptionEx)

set(BUILD_TESTING OFF CACHE BOOL "Disable Abseil's tests [default: OFF]")

option_ex(ENABLE_LTO                "Enable Link Time Optimization" ON)
option_ex(SFIZZ_JACK                "Enable JACK stand-alone build" CMAKE_SYSTEM_NAME STREQUAL "Linux")
option_ex(SFIZZ_RENDER              "Enable renderer of SMF files" ON)
option_ex(SFIZZ_BENCHMARKS          "Enable benchmarks build" OFF)
option_ex(SFIZZ_TESTS               "Enable tests build" OFF)
option_ex(SFIZZ_DEMOS               "Enable feature demos build" OFF)
option_ex(SFIZZ_DEVTOOLS            "Enable developer tools build" OFF)
option_ex(SFIZZ_SHARED              "Enable shared library build" ON)
option_ex(SFIZZ_USE_SNDFILE         "Enable use of the sndfile library" OFF)
option_ex(SFIZZ_USE_VCPKG           "Assume that sfizz is build using vcpkg" OFF)
option_ex(SFIZZ_USE_SYSTEM_ABSEIL   "Use Abseil libraries preinstalled on system" OFF)
option_ex(SFIZZ_USE_SYSTEM_GHC_FS   "Use GHC Filesystem libraries preinstalled on system" OFF)
option_ex(SFIZZ_USE_SYSTEM_SIMDE    "Use SIMDe libraries preinstalled on system" OFF)
option_ex(SFIZZ_USE_SYSTEM_KISS_FFT "Use KISS FFT libraries preinstalled on system" OFF)
option_ex(SFIZZ_USE_SYSTEM_PUGIXML  "Use pugixml libraries preinstalled on system" OFF)
option_ex(SFIZZ_USE_SYSTEM_CXXOPTS  "Use CXXOPTS libraries preinstalled on system" OFF)
option_ex(SFIZZ_USE_SYSTEM_CATCH    "Use Catch libraries preinstalled on system" OFF)
option_ex(SFIZZ_RELEASE_ASSERTS     "Forced assertions in release builds" OFF)
option_ex(SFIZZ_PROFILE_BUILD       "Profile the build time" OFF)
option_ex(SFIZZ_SNDFILE_STATIC      "Link the sndfile library statically" OFF)

# Continuous Controller count (0 to 511)
set(MIDI_CC_COUNT 512 CACHE STRING "Maximum number of managed Control Change messages")

include(SfizzConfig)
include(SfizzDeps)
include(SfizzFaust)

# Don't use IPO in non Release builds
include(CheckIPO)

# Add the static library targets and sources
add_subdirectory(src)

# Optional targets
add_subdirectory(clients)

if(SFIZZ_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(SFIZZ_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(SFIZZ_DEMOS)
    add_subdirectory(demos)
endif()

if(SFIZZ_DEVTOOLS)
    add_subdirectory(devtools)
endif()

show_build_info_if_needed()
