set (SFIZZ_SIMD_SOURCES
    sfizz/SIMDSSE.cpp
    sfizz/SIMDNEON.cpp
    sfizz/SIMDDummy.cpp)

# SIMD checks
if (CMAKE_SYSTEM_PROCESSOR STREQUAL "armv7l")
    add_compile_options (-DHAVE_ARM_NEON_H)
    add_compile_options (-mfpu=neon)
    add_compile_options (-march=native)
    add_compile_options (-mtune=cortex-a53)
endif()

set (SFIZZ_SOURCES ${SFIZZ_SOURCES} ${SFIZZ_SIMD_SOURCES})
