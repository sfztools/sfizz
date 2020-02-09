set (SFIZZ_SIMD_SOURCES
    sfizz/SIMDSSE.cpp
    sfizz/SIMDNEON.cpp
    sfizz/SIMDDummy.cpp)

list (APPEND SFIZZ_SOURCES ${SFIZZ_SIMD_SOURCES})
