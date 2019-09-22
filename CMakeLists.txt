cmake_minimum_required(VERSION 3.13)
project(sfizz VERSION 1.0.0 LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 14)

# Enable LTO
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW) # To override the policy in abseil and benchmark
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT ANDROID)
    set(USE_LIBCPP ON CACHE BOOL "Use libc++ with clang")
    add_compile_options(-stdlib=libc++)
    # Presumably need the above for linking too, maybe other options missing as well
    add_link_options(-stdlib=libc++)   # New command on CMake master, not in 3.12 release
    add_link_options(-lc++abi)   # New command on CMake master, not in 3.12 release
endif()

if (UNIX) 
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-ffast-math)
    add_compile_options(-fno-omit-frame-pointer)
endif()

if (WIN32)
    # TODO: Remove when abseil updates (deprecation warning from c++17 onwards)
    add_compile_options(/wd4996)
    # add_compile_options(/W3)
    # add_compile_options(/GR-)
    # add_compile_options(/permissive-)
    # add_compile_options(/fp:fast)
endif()

message("Cmake flags: ${CMAKE_CXX_FLAGS}")

# Export the compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# find_package(Git QUIET)
# if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
#     option(GIT_SUBMODULE "Check submodules during build" ON)
 #    if(GIT_SUBMODULE)
 #        message(STATUS "Submodule update")
 #        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --remote 
 #                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
 #                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
 #        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
#             message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
#         endif()
#     endif()
# endif()

# Build options and includes
set(BUILD_TESTING OFF CACHE BOOL "Disable Abseil's tests")
add_subdirectory(external/abseil-cpp EXCLUDE_FROM_ALL)

# Download libsndfile
if (WIN32)
    # file(DOWNLOAD https://github.com/bastibe/libsndfile-binaries/raw/master/libsndfile32bit.dll external/libsndfile-win/libsndfile32bit.dll)
    # file(DOWNLOAD https://github.com/bastibe/libsndfile-binaries/raw/master/libsndfile64bit.dll external/libsndfile-win/libsndfile64bit.dll)
    # message(STATUS "Downloading libsndfile")
    # # find_library(sndfile libsndfile-1.lib HINTS ${WIN_SNDFILE_PATH}/lib)
    set(WIN_SNDFILE_PATH "C:/Program Files/Mega-Nerd/libsndfile")
    add_library(sndfile STATIC IMPORTED)
    set_target_properties(sndfile PROPERTIES IMPORTED_LOCATION "${WIN_SNDFILE_PATH}/lib/libsndfile-1.lib")
    set_target_properties(sndfile PROPERTIES LINKER_LANGUAGE CXX)
    file(COPY "${WIN_SNDFILE_PATH}/bin/libsndfile-1.dll" DESTINATION ${CMAKE_BINARY_DIR})
    target_include_directories(sndfile INTERFACE "${WIN_SNDFILE_PATH}/include")
elseif(APPLE)
    find_path(SNDFILE_INCLUDE_DIR NAMES sndfile.hh PATHS "/usr/local/include")
    find_library(SNDFILE_LIBRARY  NAMES sndfile    PATHS "/usr/local/lib")
    message(STATUS "include dir: " ${SNDFILE_INCLUDE_DIR} " lib: " ${SNDFILE_LIBRARY})
    add_library(sndfile SHARED IMPORTED GLOBAL)
    target_include_directories(sndfile INTERFACE ${SNDFILE_INCLUDE_DIR})
    target_link_libraries(sndfile INTERFACE ${SNDFILE_LIBRARY})
endif()

add_subdirectory(sfizz)

if (SFIZZ_CLIENTS)
add_subdirectory(clients)
endif()

if (SFIZZ_TESTS)
add_subdirectory(tests)
endif()

if (SFIZZ_BENCHMARKS)
# Benchmarks
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable Google Benchmark tests")
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable Google Benchmark install")
add_subdirectory(external/benchmark EXCLUDE_FROM_ALL)
add_subdirectory(benchmarks)
endif()
