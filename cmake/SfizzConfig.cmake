# Do not override the C++ standard if set to more than 14
if (NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 14)
    set(CMAKE_CXX_STANDARD 14)
endif()

# Export the compile_commands.json file
set (CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Only install what's explicitely said
set (CMAKE_SKIP_INSTALL_ALL_DEPENDENCY true)

# Add required flags for the builds
if (UNIX)
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-ffast-math)
    add_compile_options(-fno-omit-frame-pointer) # For debugging purposes
    add_compile_options(-fPIC)
    endif()

if (WIN32)
    find_package(LibSndFile REQUIRED)
    find_path(SNDFILE_INCLUDE_DIR sndfile.hh)
    find_path(SAMPLERATE_INCLUDE_DIR samplerate.h)
    set(CMAKE_CXX_STANDARD 17)
    add_compile_options(/Zc:__cplusplus)
endif()

function(SFIZZ_LINK_LIBSNDFILE TARGET)
    if (WIN32)
        target_link_libraries (${TARGET} PRIVATE sndfile-shared)
        target_include_directories(${TARGET} PRIVATE ${SNDFILE_INCLUDE_DIR})
    else()
        target_link_libraries(${TARGET} PRIVATE sndfile)
    endif()
endfunction(SFIZZ_LINK_LIBSNDFILE)

function(SFIZZ_LINK_LIBSAMPLERATE TARGET)
    if (WIN32)
        target_link_libraries (${TARGET} PRIVATE libsamplerate-0)
        target_include_directories(${TARGET} PRIVATE ${SAMPLERATE_INCLUDE_DIR})
    else()
        target_link_libraries(${TARGET} PRIVATE samplerate)
    endif()
endfunction(SFIZZ_LINK_LIBSAMPLERATE)

# If we build with Clang use libc++
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT ANDROID)
    set(USE_LIBCPP ON CACHE BOOL "Use libc++ with clang")
    add_compile_options(-stdlib=libc++)
    # Presumably need the above for linking too, maybe other options missing as well
    add_link_options(-stdlib=libc++)   # New command on CMake master, not in 3.12 release
    add_link_options(-lc++abi)   # New command on CMake master, not in 3.12 release
endif()
