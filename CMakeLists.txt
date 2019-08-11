cmake_minimum_required(VERSION 3.13)
# You need to generate the AppConfig.h and the JuceHeader.h through the Projucer; the rest is more or less here 

project(sfizz VERSION 1.0.0 LANGUAGES CXX)

# Set the highest possible standard
set(CMAKE_CXX_STANDARD 17)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT ANDROID)
    add_compile_options(-stdlib=libc++)
    # Presumably need the above for linking too, maybe other options missing as well
    add_link_options(-stdlib=libc++)   # New command on CMake master, not in 3.12 release
endif()

if (UNIX) 
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    # add_compile_options(-fno-exceptions)
    add_compile_options(-ffast-math)
    # add_compile_options(-fno-omit-frame-pointer)
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

# Check SIMD
set(USE_SIMD OFF)
include(CheckIncludeFiles)
CHECK_INCLUDE_FILES(x86intrin.h HAVE_X86INTRIN_H)
CHECK_INCLUDE_FILES(intrin.h HAVE_INTRIN_H)
CHECK_INCLUDE_FILES(arm_neon.h HAVE_ARM_NEON_H)

# Build options and includes
set(BUILD_TESTING OFF CACHE BOOL "Disable Abseil's tests")
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Disable Google Benchmark tests")
set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "Disable Google Benchmark install")
add_subdirectory(external/abseil-cpp)
add_subdirectory(external/spdlog)
add_subdirectory(external/Catch2)
add_subdirectory(external/cxxopts)
add_subdirectory(external/gsl-lite)
add_subdirectory(external/benchmark)
add_subdirectory(external/cnpy)

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
endif()

# Export MoodyCamel's queue as a library
add_library(readerwriterqueue INTERFACE)
target_include_directories(readerwriterqueue INTERFACE "external/readerwriterqueue")

# Export the compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(COMMON_SOURCES
    sources/Opcode.cpp
    sources/Synth.cpp
    sources/FilePool.cpp
    sources/Region.cpp
    sources/Parser.cpp
)

set(SSE_SOURCES
    sources/StereoBufferSSE.cpp
)

if (HAVE_X86INTRIN_H AND UNIX)
    add_compile_options(-DHAVE_X86INTRIN_H)
    add_compile_options(-DUSE_SIMD)
    set(COMMON_SOURCES ${COMMON_SOURCES} ${SSE_SOURCES})
endif()
if (HAVE_INTRIN_H AND WIN32)
    add_compile_options(/DHAVE_INTRIN_H)
    add_compile_options(/DUSE_SIMD)
    set(COMMON_SOURCES ${COMMON_SOURCES} ${SSE_SOURCES})
endif()
if (HAVE_ARM_NEON_H AND UNIX)
    add_compile_options(-DUSE_SIMD)
    add_compile_options(-DHAVE_ARM_NEON_H)
endif()
    
set(TEST_SOURCES
    tests/RegexT.cpp
    tests/HelpersT.cpp
    tests/RegionT.cpp
    tests/RangeT.cpp
    tests/OpcodeT.cpp
    tests/BufferT.cpp
    tests/StereoBufferT.cpp
    tests/FilesT.cpp
    tests/OnePoleFilterT.cpp
    tests/RegionActivationT.cpp
    tests/MainT.cpp
)

###############################
add_executable(sfzprint sources/ParserMain.cpp sources/Opcode.cpp sources/Parser.cpp)
# Per OS properties
if(UNIX)
    target_link_libraries(sfzprint stdc++fs)
endif(UNIX)
target_link_libraries(sfzprint absl::strings cxxopts)


###############################
# Basic command line program
add_executable(sfizz sources/Main.cpp ${COMMON_SOURCES})
if (HAVE_INTRIN_H OR HAVE_X86INTRIN_H)
    target_sources(sfizz PRIVATE ${SSE_SOURCES})
endif()
# Per OS properties
if(UNIX)
    target_compile_options(sfizz PRIVATE -fno-rtti)
    target_link_libraries(sfizz stdc++fs)
endif(UNIX)
target_link_libraries(sfizz absl::strings cxxopts sndfile absl::flat_hash_map readerwriterqueue)

###############################
# Test application
add_executable(sfizz_tests ${TEST_SOURCES} ${COMMON_SOURCES})
# Per OS properties
if (HAVE_INTRIN_H OR HAVE_X86INTRIN_H)
    target_sources(sfizz_tests PRIVATE ${SSE_SOURCES})
endif()
if(UNIX)
    target_link_libraries(sfizz_tests stdc++fs)
endif(UNIX)
target_link_libraries(sfizz_tests Catch2::Catch2 absl::strings absl::flat_hash_map sndfile readerwriterqueue cnpy-static gsl::gsl-lite)
target_include_directories(sfizz_tests SYSTEM PRIVATE sources)
file(COPY "tests" DESTINATION ${CMAKE_BINARY_DIR})

###############################
add_executable(bench_fill benchmarks/Stereo_Fill.cpp sources/StereoBufferSSE.cpp)
# Per OS properties
target_link_libraries(bench_fill benchmark)

###############################
add_executable(bench_read benchmarks/Stereo_Read_Interleaved.cpp sources/StereoBufferSSE.cpp)
# Per OS properties
target_link_libraries(bench_read benchmark)

###############################
add_executable(bench_write benchmarks/Stereo_Write_Interleaved.cpp sources/StereoBufferSSE.cpp)
# Per OS properties
target_link_libraries(bench_write benchmark)

###############################
add_executable(bench_span benchmarks/Spans.cpp)
# Per OS properties
target_link_libraries(bench_span benchmark gsl::gsl-lite absl::span)

###############################
add_executable(bench_opf_high_vs_low benchmarks/OPF_high_vs_low.cpp)
# Per OS properties
target_link_libraries(bench_opf_high_vs_low benchmark gsl::gsl-lite)

###############################
add_executable(bench_looping_index benchmarks/Looping_index.cpp)
# Per OS properties
target_link_libraries(bench_looping_index benchmark)

add_executable(bench_looping_index_2 benchmarks/Looping_index_2.cpp)
# Per OS properties
target_link_libraries(bench_looping_index_2 benchmark)
