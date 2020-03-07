if (WIN32)
    cmake_minimum_required (VERSION 3.15)
    cmake_policy(SET CMP0091 NEW)
else()
    cmake_minimum_required (VERSION 3.5)
    if (POLICY CMP0069)
        cmake_policy(SET CMP0069 NEW)
    endif()
endif()

project (sfizz VERSION 0.3.0 LANGUAGES CXX C)
set (PROJECT_DESCRIPTION "A library to load SFZ description files and use them to render music.")

# External configuration CMake scripts
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (SfizzConfig)

# Build Options
set (BUILD_TESTING OFF CACHE BOOL "Disable Abseil's tests [default: OFF]")

# On macOS add the needed directories for both library and jack client
if (APPLE)
    include_directories (SYSTEM /usr/local/opt/libsndfile/include)
    link_directories (/usr/local/opt/libsndfile/lib)
endif()

option (ENABLE_LTO              "Enable Link Time Optimization [default: ON]" ON)
option (SFIZZ_JACK              "Enable JACK stand-alone build [default: ON]" ON)
option (SFIZZ_LV2               "Enable LV2 plug-in build [default: ON]" ON)
option (SFIZZ_VST               "Enable VST plug-in build [default: OFF]" OFF)
option (SFIZZ_BENCHMARKS        "Enable benchmarks build [default: OFF]" OFF)
option (SFIZZ_TESTS             "Enable tests build [default: OFF]" OFF)
option (SFIZZ_SHARED            "Enable shared library build [default: ON]" ON)
option (SFIZZ_USE_VCPKG         "Assume that sfizz is build using vcpkg [default: OFF]" OFF)
option (SFIZZ_STATIC_LIBSNDFILE "Link libsndfile statically [default: OFF]" OFF)

# Don't use IPO in non Release builds
include (CheckIPO)

# Add Abseil
add_subdirectory (external/abseil-cpp EXCLUDE_FROM_ALL)

# Add the static library targets and sources
add_subdirectory (src)

# Optional targets
if (SFIZZ_JACK)
    add_subdirectory (clients)
endif()

if (SFIZZ_LV2)
    add_subdirectory (lv2)
endif()

if (SFIZZ_VST)
    add_subdirectory (vst)
endif()

if (SFIZZ_BENCHMARKS)
    add_subdirectory (benchmarks)
endif()

if (SFIZZ_TESTS)
    add_subdirectory (tests)
endif()

show_build_info_if_needed()
