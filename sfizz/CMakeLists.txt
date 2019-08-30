project(sfizz)

set(SFIZZ_SOURCES
    Synth.cpp
    FilePool.cpp
    Region.cpp
    Voice.cpp
    ScopedFTZ.cpp
    SfzHelpers.cpp
    FloatEnvelopes.cpp
)

# Check SIMD
include(CheckIncludeFiles)
CHECK_INCLUDE_FILES(x86intrin.h HAVE_X86INTRIN_H)
CHECK_INCLUDE_FILES(intrin.h HAVE_INTRIN_H)
CHECK_INCLUDE_FILES(arm_neon.h HAVE_ARM_NEON_H)

# SIMD checks
if (HAVE_X86INTRIN_H AND UNIX)
    add_compile_options(-DHAVE_X86INTRIN_H)
    set(SFIZZ_SIMD_SOURCES SIMDSSE.cpp)
elseif (HAVE_INTRIN_H AND WIN32)
    add_compile_options(/DHAVE_INTRIN_H)
    set(SFIZZ_SIMD_SOURCES SIMDSSE.cpp)
elseif (HAVE_ARM_NEON_H AND UNIX)
    add_compile_options(-DHAVE_ARM_NEON_H)
    add_compile_options(-mfpu=neon-fp-armv8)
    add_compile_options(-march=native)
    add_compile_options(-mtune=cortex-a53)
    add_compile_options(-funsafe-math-optimizations)
    set(SFIZZ_SIMD_SOURCES SIMDNEON.cpp)
else()
    set(SFIZZ_SIMD_SOURCES SIMDDummy.cpp)
endif()
set(SFIZZ_SOURCES ${SFIZZ_SOURCES} ${SFIZZ_SIMD_SOURCES})

add_library(sfizz_parser STATIC)
target_sources(sfizz_parser PUBLIC Parser.cpp Opcode.cpp)
target_include_directories(sfizz_parser PUBLIC .)
if(UNIX)
    target_link_libraries(sfizz_parser PUBLIC stdc++fs)
endif(UNIX)
target_link_libraries(sfizz_parser PRIVATE absl::strings)

add_library(sfizz STATIC ${SFIZZ_SOURCES})
target_link_libraries(sfizz PRIVATE sfizz_parser)
target_include_directories(sfizz PUBLIC .)
if(UNIX)
    target_link_libraries(sfizz PUBLIC stdc++fs atomic)
    target_compile_options(sfizz PRIVATE -fno-rtti -fno-exceptions)
endif(UNIX)
target_link_libraries(sfizz PUBLIC readerwriterqueue)
target_link_libraries(sfizz PRIVATE absl::strings sndfile absl::flat_hash_map)

add_library(sfizz::parser ALIAS sfizz_parser)
add_library(sfizz::sfizz ALIAS sfizz)