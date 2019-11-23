cmake_minimum_required(VERSION 3.13)
project(sfizz VERSION 1.0.0 LANGUAGES CXX)

# Do not override the C++ standard if set to more than 14
if (NOT CMAKE_CXX_STANDARD OR CMAKE_CXX_STANDARD LESS 14)
    set(CMAKE_CXX_STANDARD 14)
endif()

# Export the compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add required flags for the builds
if (UNIX)
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-ffast-math)
    add_compile_options(-fno-omit-frame-pointer) # For debugging purposes
    add_compile_options(-fPIC)
endif()

# If we build with Clang use libc++
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT ANDROID)
    set(USE_LIBCPP ON CACHE BOOL "Use libc++ with clang")
    add_compile_options(-stdlib=libc++)
    # Presumably need the above for linking too, maybe other options missing as well
    add_link_options(-stdlib=libc++)   # New command on CMake master, not in 3.12 release
    add_link_options(-lc++abi)   # New command on CMake master, not in 3.12 release
endif()

# Add Abseil
set(BUILD_TESTING OFF CACHE BOOL "Disable Abseil's tests")
add_subdirectory(external/abseil-cpp EXCLUDE_FROM_ALL)

# Add the static library targets and sources
add_subdirectory(src)
