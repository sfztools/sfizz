include(CMakeDependentOption)
include(CheckCXXCompilerFlag)
include(GNUWarnings)

set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ standard to be used")
set(CMAKE_C_STANDARD 99 CACHE STRING "C standard to be used")

# Export the compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Only install what's explicitely said
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Set C++ compatibility level
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_CXX_STANDARD LESS 17)
    set(CMAKE_CXX_STANDARD 17)
elseif((SFIZZ_LV2_UI OR SFIZZ_VST OR SFIZZ_AU OR SFIZZ_VST2) AND CMAKE_CXX_STANDARD LESS 14)
    # if the UI is part of the build, make it 14
    set(CMAKE_CXX_STANDARD 14)
endif()

# Process sources as UTF-8
if(MSVC)
    add_compile_options("/utf-8")
endif()

# Set Windows compatibility level to 7
if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x601)
endif()

# Set macOS compatibility level
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
endif()

# Do not define macros `min` and `max`
if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

# The variable CMAKE_SYSTEM_PROCESSOR is incorrect on Visual studio...
# see https://gitlab.kitware.com/cmake/cmake/issues/15170

if(NOT SFIZZ_SYSTEM_PROCESSOR)
    if(MSVC)
        set(SFIZZ_SYSTEM_PROCESSOR "${MSVC_CXX_ARCHITECTURE_ID}")
    else()
        set(SFIZZ_SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}")
    endif()
endif()

# Add required flags for the builds
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    gw_warn(-Wall -Wextra -Wno-multichar -Werror=return-type)
    if(SFIZZ_SYSTEM_PROCESSOR MATCHES "^(i.86|x86_64)$")
        add_compile_options(-msse2)
    elseif(SFIZZ_SYSTEM_PROCESSOR MATCHES "^(arm.*)$")
        add_compile_options(-mfpu=neon)
        if(NOT ANDROID)
            add_compile_options(-mfloat-abi=hard)
        endif()
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    add_compile_options(/Zc:__cplusplus)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

function(sfizz_enable_fast_math NAME)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options("${NAME}" PRIVATE "-ffast-math")
    elseif(MSVC)
        target_compile_options("${NAME}" PRIVATE "/fp:fast")
    endif()
endfunction()

# If we build with Clang, optionally use libc++. Enabled by default on Apple OS.
cmake_dependent_option(USE_LIBCPP "Use libc++ with clang" "${APPLE}"
    "CMAKE_CXX_COMPILER_ID MATCHES Clang" OFF)
if(USE_LIBCPP)
    add_compile_options(-stdlib=libc++)
    # Presumably need the above for linking too, maybe other options missing as well
    add_link_options(-stdlib=libc++)   # New command on CMake master, not in 3.12 release
    add_link_options(-lc++abi)   # New command on CMake master, not in 3.12 release
endif()

# Don't show build information when building a different project
function(show_build_info_if_needed)
    if(CMAKE_PROJECT_NAME STREQUAL "sfizz")
        message(STATUS "
Project name:                  ${PROJECT_NAME}
Build type:                    ${CMAKE_BUILD_TYPE}
Build processor:               ${SFIZZ_SYSTEM_PROCESSOR}
Build using LTO:               ${ENABLE_LTO}
Build as shared library:       ${SFIZZ_SHARED}
Build JACK stand-alone client: ${SFIZZ_JACK}
Build render client:           ${SFIZZ_RENDER}
Build LV2 plug-in:             ${SFIZZ_LV2}
Build LV2 user interface:      ${SFIZZ_LV2_UI}
Build VST plug-in:             ${SFIZZ_VST}
Build AU plug-in:              ${SFIZZ_AU}
Build benchmarks:              ${SFIZZ_BENCHMARKS}
Build tests:                   ${SFIZZ_TESTS}
Build demos:                   ${SFIZZ_DEMOS}
Build devtools:                ${SFIZZ_DEVTOOLS}
Use sndfile:                   ${SFIZZ_USE_SNDFILE}
Use vcpkg:                     ${SFIZZ_USE_VCPKG}
Statically link dependencies:  ${SFIZZ_STATIC_DEPENDENCIES}
Use clang libc++:              ${USE_LIBCPP}
Release asserts:               ${SFIZZ_RELEASE_ASSERTS}

Install prefix:                ${CMAKE_INSTALL_PREFIX}
LV2 destination directory:     ${LV2PLUGIN_INSTALL_DIR}

Compiler CXX debug flags:      ${CMAKE_CXX_FLAGS_DEBUG}
Compiler CXX release flags:    ${CMAKE_CXX_FLAGS_RELEASE}
Compiler CXX min size flags:   ${CMAKE_CXX_FLAGS_MINSIZEREL}
")
    endif()
endfunction()
