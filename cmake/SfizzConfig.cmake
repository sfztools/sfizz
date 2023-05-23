include(CMakeDependentOption)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)
include(GNUWarnings)

# C++14 is the minimum standard version required by Abseil LTS 20230125.1 and later, see
# https://github.com/abseil/abseil-cpp/releases/tag/20230125.1
set(CMAKE_CXX_STANDARD 14 CACHE STRING "C++ standard to be used")
set(CMAKE_C_STANDARD 99 CACHE STRING "C standard to be used")

# Export the compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Only install what's explicitely said
set(CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/library/bin)

# Set C++ compatibility level
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_CXX_STANDARD LESS 17)
    set(CMAKE_CXX_STANDARD 17)
endif()

# Set build profiling options
if(SFIZZ_PROFILE_BUILD)
    check_c_compiler_flag("-ftime-trace" SFIZZ_HAVE_CFLAG_FTIME_TRACE)
    check_cxx_compiler_flag("-ftime-trace" SFIZZ_HAVE_CXXFLAG_FTIME_TRACE)
    if(SFIZZ_HAVE_CFLAG_FTIME_TRACE)
        add_compile_options("$<$<COMPILE_LANGUAGE:C>:-ftime-trace>")
    endif()
    if(SFIZZ_HAVE_CXXFLAG_FTIME_TRACE)
        add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-ftime-trace>")
    endif()
endif()

# Process sources as UTF-8
if(MSVC)
    add_compile_options("/utf-8")
endif()

# Define the math constants everywhere
if(WIN32)
    add_compile_definitions(_USE_MATH_DEFINES)
endif()

# Set macOS compatibility level
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
endif()

# If using C++17, check if aligned-new has runtime support on the platform;
# on macOS, this depends on the deployment target.
if(CMAKE_CXX_STANDARD LESS 17)
    # not necessary on older C++, it will call ordinary new and delete
    set(SFIZZ_IMPLEMENT_CXX17_ALIGNED_NEW_SUPPORT FALSE)
else()
    check_cxx_source_compiles("
struct Test { alignas(1024) int z; };
int main() { new Test; return 0; }
" SFIZZ_HAVE_CXX17_ALIGNED_NEW)
    # if support is absent, sfizz will provide a substitute implementation
    if(SFIZZ_HAVE_CXX17_ALIGNED_NEW)
        set(SFIZZ_IMPLEMENT_CXX17_ALIGNED_NEW_SUPPORT FALSE)
    else()
        # on macOS, this mandatory flag tells that allocation functions are user-provided
        check_cxx_compiler_flag("-faligned-allocation" SFIZZ_HAVE_CXXFLAG_FALIGNED_ALLOCATION)
        if(SFIZZ_HAVE_CXXFLAG_FALIGNED_ALLOCATION)
            add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-faligned-allocation>")
        endif()
        set(SFIZZ_IMPLEMENT_CXX17_ALIGNED_NEW_SUPPORT TRUE)
    endif()
endif()

# Do not define macros `min` and `max`
if(WIN32)
    add_compile_definitions(NOMINMAX)
endif()

# The variable CMAKE_SYSTEM_PROCESSOR is incorrect on Visual studio...
# see https://gitlab.kitware.com/cmake/cmake/issues/15170
if(NOT PROJECT_SYSTEM_PROCESSOR)
    if(MSVC)
        set(PROJECT_SYSTEM_PROCESSOR "${MSVC_CXX_ARCHITECTURE_ID}" CACHE STRING "" FORCE)
    else()
        set(PROJECT_SYSTEM_PROCESSOR "${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "" FORCE)
    endif()
endif()

# Add required flags for the builds
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    gw_warn(-Wall -Wextra -Wno-multichar -Werror=return-type)
    if(PROJECT_SYSTEM_PROCESSOR MATCHES "^(i.86|x86_64)$")
        add_compile_options(-msse2)
    elseif(PROJECT_SYSTEM_PROCESSOR MATCHES "^(arm.*)$")
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
    add_link_options(-stdlib=libc++) # New command in CMake 3.13
    add_link_options(-lc++abi)
endif()

include(GNUInstallDirs)

if(PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(PROJECT_IS_MAIN TRUE)
else()
    set(PROJECT_IS_MAIN FALSE)
endif()

# Don't show build information when building a different project
function(show_build_info_if_needed)
    if(PROJECT_IS_MAIN)
        message(STATUS "
Project name:                  ${PROJECT_NAME}
C++ standard version:          ${CMAKE_CXX_STANDARD}
Build type:                    ${CMAKE_BUILD_TYPE}
Build processor:               ${PROJECT_SYSTEM_PROCESSOR}
Build using LTO:               ${ENABLE_LTO}
Build as shared library:       ${SFIZZ_SHARED}
Build JACK stand-alone client: ${SFIZZ_JACK}
Build render client:           ${SFIZZ_RENDER}
Build benchmarks:              ${SFIZZ_BENCHMARKS}
Build tests:                   ${SFIZZ_TESTS}
Build demos:                   ${SFIZZ_DEMOS}
Build devtools:                ${SFIZZ_DEVTOOLS}

Use sndfile:                   ${SFIZZ_USE_SNDFILE}
Statically link sndfile:       ${SFIZZ_SNDFILE_STATIC}

Use vcpkg:                     ${SFIZZ_USE_VCPKG}
Use clang libc++:              ${USE_LIBCPP}
Release asserts:               ${SFIZZ_RELEASE_ASSERTS}

Use system abseil-cpp:         ${SFIZZ_USE_SYSTEM_ABSEIL}
Use system catch:              ${SFIZZ_USE_SYSTEM_CATCH}
Use system cxxopts:            ${SFIZZ_USE_SYSTEM_CXXOPTS}
Use system ghc-filesystem:     ${SFIZZ_USE_SYSTEM_GHC_FS}
Use system kiss-fft:           ${SFIZZ_USE_SYSTEM_KISS_FFT}
Use system pugixml:            ${SFIZZ_USE_SYSTEM_PUGIXML}
Use system simde:              ${SFIZZ_USE_SYSTEM_SIMDE}")
        if(CMAKE_PROJECT_NAME STREQUAL "sfizz")
            message(STATUS "
Use system lv2:                ${SFIZZ_USE_SYSTEM_LV2}
Use system vst3sdk sources:    ${SFIZZ_USE_SYSTEM_VST3SDK}

Build AU plug-in:              ${PLUGIN_AU}
Build LV2 plug-in:             ${PLUGIN_LV2}
Build LV2 user interface:      ${PLUGIN_LV2_UI}
LV2 plugin-side CC automation  ${PLUGIN_LV2_PSA}
Build Pure Data plug-in:       ${PLUGIN_PUREDATA}
Build VST3 plug-in:            ${PLUGIN_VST3}

AU   destination directory:    ${AU_PLUGIN_INSTALL_DIR}
LV2  destination directory:    ${LV2_PLUGIN_INSTALL_DIR}
Pd   destination directory:    ${PD_PLUGIN_INSTALL_DIR}
VST3 destination directory:    ${VST3_PLUGIN_INSTALL_DIR}")
        endif()
        message(STATUS "
Install prefix:                ${CMAKE_INSTALL_PREFIX}

CXX Debug flags:               ${CMAKE_CXX_FLAGS_DEBUG}
CXX Release flags:             ${CMAKE_CXX_FLAGS_RELEASE}
CXX MinSize flags:             ${CMAKE_CXX_FLAGS_MINSIZEREL}
CXX RelWithDebInfo flags:      ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}
")
    endif()
endfunction()
