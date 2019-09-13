project(sfizz)

# Check SIMD
include(CheckIncludeFiles)
CHECK_INCLUDE_FILES(x86intrin.h HAVE_X86INTRIN_H)
CHECK_INCLUDE_FILES(intrin.h HAVE_INTRIN_H)
CHECK_INCLUDE_FILES(arm_neon.h HAVE_ARM_NEON_H)

# SIMD checks
if (HAVE_X86INTRIN_H AND UNIX)
    add_compile_options(-DHAVE_X86INTRIN_H)
    set(SFIZZ_SIMD_SOURCES ../sfizz/SIMDSSE.cpp)
elseif (HAVE_INTRIN_H AND WIN32)
    add_compile_options(/DHAVE_INTRIN_H)
    set(SFIZZ_SIMD_SOURCES ../sfizz/SIMDSSE.cpp)
elseif (HAVE_ARM_NEON_H AND UNIX)
    add_compile_options(-DHAVE_ARM_NEON_H)
    add_compile_options(-mfpu=neon-fp-armv8)
    add_compile_options(-march=native)
    add_compile_options(-mtune=cortex-a53)
    add_compile_options(-funsafe-math-optimizations)
    set(SFIZZ_SIMD_SOURCES ../sfizz/SIMDNEON.cpp)
else()
    set(SFIZZ_SIMD_SOURCES ../sfizz/SIMDDummy.cpp)
endif()

add_executable(bm_opf_high_vs_low BM_OPF_high_vs_low.cpp)
target_link_libraries(bm_opf_high_vs_low benchmark absl::span)

add_executable(bm_write BM_writeInterleaved.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_write benchmark absl::span)

add_executable(bm_read BM_readInterleaved.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_read benchmark absl::span)

add_executable(bm_fill BM_fill.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_fill benchmark absl::span)

add_executable(bm_mathfuns BM_mathfuns.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_mathfuns benchmark absl::span)

add_executable(bm_gain BM_gain.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_gain benchmark absl::span)

add_executable(bm_looping BM_looping.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_looping benchmark absl::span absl::algorithm)

add_executable(bm_saturating BM_saturating.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_saturating benchmark absl::span absl::algorithm)

add_executable(bm_ramp BM_ramp.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_ramp benchmark absl::span absl::algorithm)

add_executable(bm_ADSR BM_ADSR.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_ADSR benchmark absl::span absl::algorithm)

add_executable(bm_add BM_add.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_add benchmark absl::span absl::algorithm)

add_executable(bm_multiplyAdd BM_multiplyAdd.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_multiplyAdd benchmark absl::span absl::algorithm)

add_executable(bm_subtract BM_subtract.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_subtract benchmark absl::span absl::algorithm)

add_executable(bm_copy BM_copy.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_copy benchmark absl::span absl::algorithm)

add_executable(bm_pan BM_pan.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_pan benchmark absl::span absl::algorithm)

add_executable(bm_mean BM_mean.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_mean benchmark absl::span absl::algorithm)

add_executable(bm_meanSquared BM_meanSquared.cpp ${SFIZZ_SIMD_SOURCES})
target_link_libraries(bm_meanSquared benchmark absl::span absl::algorithm)

add_custom_target(sfizz_benchmarks)
add_dependencies(sfizz_benchmarks 
	bm_opf_high_vs_low 
	bm_write
	bm_read
	bm_mean
	bm_meanSquared
	bm_fill
	bm_mathfuns
	bm_gain
	bm_looping
	bm_saturating
	bm_ramp
	bm_ADSR
	bm_add
	bm_pan
	bm_subtract
	bm_multiplyAdd
)