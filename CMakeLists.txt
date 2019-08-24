cmake_minimum_required(VERSION 3.13)
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
    # add_compile_options(-fno-rtti)
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
add_subdirectory(external/Catch2)
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
    sources/FloatEnvelopes.cpp
    # sources/LinearEnvelope.cpp
    # sources/ADSREnvelope.cpp
    sources/Parser.cpp
)

set(SSE_SOURCES
    
)

if (HAVE_X86INTRIN_H AND UNIX)
    add_compile_options(-DHAVE_X86INTRIN_H)
    add_compile_options(-DUSE_SIMD)
    set(COMMON_SOURCES ${COMMON_SOURCES} sources/SIMDSSE.cpp)
elseif (HAVE_INTRIN_H AND WIN32)
    add_compile_options(/DHAVE_INTRIN_H)
    add_compile_options(/DUSE_SIMD)
    set(COMMON_SOURCES ${COMMON_SOURCES} sources/SIMDSSE.cpp)
elseif (HAVE_ARM_NEON_H AND UNIX)
    add_compile_options(-DUSE_SIMD)
    add_compile_options(-DHAVE_ARM_NEON_H)
    set(COMMON_SOURCES ${COMMON_SOURCES} sources/SIMDDummy.cpp)
else()
    set(COMMON_SOURCES ${COMMON_SOURCES} sources/SIMDDummy.cpp)
endif()
    
set(TEST_SOURCES
    tests/RegexT.cpp
    tests/HelpersT.cpp
    tests/RegionT.cpp
    tests/RangeT.cpp
    tests/OpcodeT.cpp
    tests/BufferT.cpp
    tests/StereoBufferT.cpp
    tests/SIMDHelpersT.cpp
    tests/FilesT.cpp
    tests/OnePoleFilterT.cpp
    tests/RegionActivationT.cpp
    tests/ADSREnvelopeT.cpp
    tests/LinearEnvelopeT.cpp
    tests/MainT.cpp
    tests/RegionTriggersT.cpp
)

###############################
add_executable(sfzprint sources/ParserMain.cpp sources/Opcode.cpp sources/Parser.cpp)
# Per OS properties
if(UNIX)
    target_link_libraries(sfzprint stdc++fs)
endif(UNIX)
target_link_libraries(sfzprint absl::strings absl::flags_parse)


###############################
# Basic command line program
add_executable(sfizz sources/Main.cpp ${COMMON_SOURCES})
# Per OS properties
if(UNIX)
    target_compile_options(sfizz PRIVATE -fno-rtti -fno-exceptions)
    target_link_libraries(sfizz stdc++fs)
endif(UNIX)
target_link_libraries(sfizz jack absl::strings sndfile absl::flat_hash_map readerwriterqueue absl::flags_parse)

###############################
# Test application
add_executable(sfizz_tests ${TEST_SOURCES} ${COMMON_SOURCES})
# Per OS properties
if(UNIX)
    target_link_libraries(sfizz_tests stdc++fs)
endif(UNIX)
target_link_libraries(sfizz_tests Catch2::Catch2 absl::strings absl::flat_hash_map sndfile readerwriterqueue cnpy-static absl::span absl::algorithm)
target_include_directories(sfizz_tests SYSTEM PRIVATE sources)
file(COPY "tests" DESTINATION ${CMAKE_BINARY_DIR})

###############################
add_executable(bench_opf_high_vs_low benchmarks/OPF_high_vs_low.cpp)
target_link_libraries(bench_opf_high_vs_low benchmark absl::span)

###############################

add_executable(bm_write benchmarks/BM_writeInterleaved.cpp sources/SIMDSSE.cpp)
target_link_libraries(bm_write benchmark absl::span)

add_executable(bm_read benchmarks/BM_readInterleaved.cpp sources/SIMDSSE.cpp)
target_link_libraries(bm_read benchmark absl::span)

add_executable(bm_fill benchmarks/BM_fill.cpp sources/SIMDSSE.cpp)
target_link_libraries(bm_fill benchmark absl::span)

add_executable(bm_mathfuns benchmarks/BM_mathfuns.cpp sources/SIMDSSE.cpp)
target_link_libraries(bm_mathfuns benchmark absl::span)

add_executable(bm_gain benchmarks/BM_gain.cpp sources/SIMDSSE.cpp)
target_link_libraries(bm_gain benchmark absl::span)

add_executable(bm_looping benchmarks/BM_looping.cpp sources/SIMDSSE.cpp)
target_link_libraries(bm_looping benchmark absl::span absl::algorithm)

add_executable(bm_ramp benchmarks/BM_ramp.cpp sources/SIMDSSE.cpp)
target_link_libraries(bm_ramp benchmark absl::span absl::algorithm)

add_executable(bm_ADSR benchmarks/BM_ADSR.cpp sources/SIMDSSE.cpp)
target_link_libraries(bm_ADSR benchmark absl::span absl::algorithm)

add_executable(bm_add benchmarks/BM_add.cpp sources/SIMDSSE.cpp)
target_link_libraries(bm_add benchmark absl::span absl::algorithm)