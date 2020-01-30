# Check SIMD
include (CheckIncludeFiles)
CHECK_INCLUDE_FILES(x86intrin.h HAVE_X86INTRIN_H)
CHECK_INCLUDE_FILES(intrin.h HAVE_INTRIN_H)

if (!APPLE)
CHECK_INCLUDE_FILES (arm_neon.h HAVE_ARM_NEON_H)
endif()

# SIMD checks
if (HAVE_X86INTRIN_H AND UNIX)
    add_compile_options (-DHAVE_X86INTRIN_H)
    set (SFIZZ_SIMD_SOURCES sfizz/SIMDSSE.cpp)
elseif (HAVE_INTRIN_H AND WIN32)
    add_compile_options (/DHAVE_INTRIN_H)
    set (SFIZZ_SIMD_SOURCES sfizz/SIMDSSE.cpp)
elseif (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
    add_compile_options (-DHAVE_ARM_NEON_H)
    add_compile_options (-mfpu=neon)
    add_compile_options (-march=native)
    add_compile_options (-mtune=cortex-a53)
    set (SFIZZ_SIMD_SOURCES sfizz/SIMDNEON.cpp)
else()
    set (SFIZZ_SIMD_SOURCES sfizz/SIMDDummy.cpp)
endif()

set (SFIZZ_SOURCES ${SFIZZ_SOURCES} ${SFIZZ_SIMD_SOURCES})
