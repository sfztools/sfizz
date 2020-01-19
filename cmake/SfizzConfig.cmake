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

# Add required flags for the builds
if (UNIX)
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-ffast-math)
    add_compile_options(-fno-omit-frame-pointer) # For debugging purposes
endif()

if (WIN32)
    find_package(LibSndFile REQUIRED)
    find_path(SNDFILE_INCLUDE_DIR sndfile.hh)
    set(CMAKE_CXX_STANDARD 17)
    add_compile_options(/Zc:__cplusplus)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

function(SFIZZ_LINK_LIBSNDFILE TARGET)
    if (WIN32)
        target_link_libraries (${TARGET} PRIVATE sndfile-static)
        target_include_directories(${TARGET} PUBLIC ${SNDFILE_INCLUDE_DIR})
    else()
        target_link_libraries(${TARGET} PRIVATE sndfile)
    endif()
endfunction(SFIZZ_LINK_LIBSNDFILE)

# If we build with Clang use libc++
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT ANDROID)
    set(USE_LIBCPP ON CACHE BOOL "Use libc++ with clang")
    add_compile_options(-stdlib=libc++)
    # Presumably need the above for linking too, maybe other options missing as well
    add_link_options(-stdlib=libc++)   # New command on CMake master, not in 3.12 release
    add_link_options(-lc++abi)   # New command on CMake master, not in 3.12 release
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

Install prefix:                ${CMAKE_INSTALL_PREFIX}
LV2 destination directory:     ${LV2PLUGIN_INSTALL_DIR}

Compiler CXX debug flags:      ${CMAKE_CXX_FLAGS_DEBUG}
Compiler CXX release flags:    ${CMAKE_CXX_FLAGS_RELEASE}
Compiler CXX min size flags:   ${CMAKE_CXX_FLAGS_MINSIZEREL}
")
    endif()
endfunction()

find_package (Threads REQUIRED)
