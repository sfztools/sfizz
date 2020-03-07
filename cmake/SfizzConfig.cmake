# Do not override the C++ standard if set to more than 14
if (NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 14)
    set(CMAKE_CXX_STANDARD 14)
endif()

# Export the compile_commands.json file
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Only install what's explicitely said
set (CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)
set (CMAKE_POSITION_INDEPENDENT_CODE ON)
set (CMAKE_CXX_VISIBILITY_PRESET hidden)
set (CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Set Windows compatibility level to 7
if (WIN32)
    add_compile_definitions(_WIN32_WINNT=0x601)
endif()

# Add required flags for the builds
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-ffast-math)
    add_compile_options(-fno-omit-frame-pointer) # For debugging purposes
elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_CXX_STANDARD 17)
    add_compile_options(/Zc:__cplusplus)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if (CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
    if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-msse2)
    endif()
endif()

add_library(sfizz-sndfile INTERFACE)

if (SFIZZ_USE_VCPKG OR CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    find_package(LibSndFile REQUIRED)
    find_path(SNDFILE_INCLUDE_DIR sndfile.hh)
    target_include_directories(sfizz-sndfile INTERFACE "${SNDFILE_INCLUDE_DIR}")
    target_link_libraries(sfizz-sndfile INTERFACE sndfile-static)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SNDFILE "sndfile" REQUIRED)
    target_include_directories(sfizz-sndfile INTERFACE ${SNDFILE_INCLUDE_DIRS})
    if (SFIZZ_STATIC_LIBSNDFILE)
        target_link_libraries(sfizz-sndfile INTERFACE ${SNDFILE_STATIC_LIBRARIES})
    else()
        target_link_libraries(sfizz-sndfile INTERFACE ${SNDFILE_LIBRARIES})
    endif()
endif()

add_library(sfizz-pugixml STATIC "src/external/pugixml/src/pugixml.cpp")
target_include_directories(sfizz-pugixml PUBLIC "src/external/pugixml/src")

# If we build with Clang use libc++
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT ANDROID)
    set(USE_LIBCPP ON CACHE BOOL "Use libc++ with clang")
    if (USE_LIBCPP)
        add_compile_options(-stdlib=libc++)
        # Presumably need the above for linking too, maybe other options missing as well
        add_link_options(-stdlib=libc++)   # New command on CMake master, not in 3.12 release
        add_link_options(-lc++abi)   # New command on CMake master, not in 3.12 release
    endif()
endif()

# Don't show build information when building a different project
function (show_build_info_if_needed)
    if (CMAKE_PROJECT_NAME STREQUAL "sfizz")
        message (STATUS "
Project name:                  ${PROJECT_NAME}
Build type:                    ${CMAKE_BUILD_TYPE}
Build using LTO:               ${ENABLE_LTO}
Build as shared library:       ${SFIZZ_SHARED}
Build JACK stand-alone client: ${SFIZZ_JACK}
Build LV2 plug-in:             ${SFIZZ_LV2}
Build benchmarks:              ${SFIZZ_BENCHMARKS}
Build tests:                   ${SFIZZ_TESTS}
Use vcpkg:                     ${SFIZZ_USE_VCPKG}
Statically link libsndfile:    ${SFIZZ_STATIC_LIBSNDFILE}

Install prefix:                ${CMAKE_INSTALL_PREFIX}
LV2 destination directory:     ${LV2PLUGIN_INSTALL_DIR}

Compiler CXX debug flags:      ${CMAKE_CXX_FLAGS_DEBUG}
Compiler CXX release flags:    ${CMAKE_CXX_FLAGS_RELEASE}
Compiler CXX min size flags:   ${CMAKE_CXX_FLAGS_MINSIZEREL}
")
    endif()
endfunction()

find_package (Threads REQUIRED)
