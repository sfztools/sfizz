cmake_minimum_required(VERSION 3.13)
project (sfizz VERSION 1.0.0 LANGUAGES CXX C)

# External configuration CMake scripts
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include (SfizzConfig)

# Build Options
set (BUILD_TESTING OFF CACHE BOOL "Disable Abseil's tests [default: OFF]")

set (LV2PLUGIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib/lv2" CACHE STRING
    "Install destination for LV2 bundle [default: ${CMAKE_INSTALL_PREFIX}/lib/lv2]")

# On macOS add the needed directories for both library and jack client
if (APPLE)
    include_directories (SYSTEM /usr/local/opt/libsndfile/include)
    link_directories (/usr/local/opt/libsndfile/lib)
endif()

option (ENABLE_LTO       "Enable Link Time Optimization [default: ON]" ON)
option (SFIZZ_JACK       "Enable JACK stand-alone build [default: ON]" ON)
option (SFIZZ_LV2        "Enable LV2 plug-in build [default: ON]" ON)
option (SFIZZ_BENCHMARKS "Enable benchmarks build [default: OFF]" OFF)
option (SFIZZ_TESTS      "Enable tests build [default: OFF]" OFF)
option (SFIZZ_SHARED     "Enable shared library build [default: ON]" ON)

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

if (SFIZZ_BENCHMARKS)
    add_subdirectory (benchmarks)
endif()

if (SFIZZ_TESTS)
    add_subdirectory (tests)
endif()

message (STATUS "
Project name:                  ${CMAKE_PROJECT_NAME}
Build type:                    ${CMAKE_BUILD_TYPE}
Build using LTO:               ${ENABLE_LTO}
Build as shared library:       ${SFIZZ_SHARED}
Build JACK stand-alone client: ${SFIZZ_JACK}
Build LV2 plug-in:             ${SFIZZ_LV2}
Build benchmarks:              ${SFIZZ_BENCHMARKS}
Build tests:                   ${SFIZZ_TESTS}

Install prefix:                ${CMAKE_INSTALL_PREFIX}
LV2 destination directory:     ${LV2PLUGIN_INSTALL_DIR}

Compiler CXX debug flags:      ${CMAKE_CXX_FLAGS_DEBUG}
Compiler CXX release flags:    ${CMAKE_CXX_FLAGS_RELEASE}
Compiler CXX min size flags:   ${CMAKE_CXX_FLAGS_MINSIZEREL}
")
