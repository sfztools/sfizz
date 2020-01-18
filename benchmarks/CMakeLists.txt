project(sfizz)

# Check SIMD
include (SfizzSIMDSourceFilesCheck)
set(BENCHMARK_SIMD_SOURCES ${SFIZZ_SIMD_SOURCES})
list(TRANSFORM BENCHMARK_SIMD_SOURCES PREPEND "../src/")

add_executable(bm_opf_high_vs_low BM_OPF_high_vs_low.cpp)
target_link_libraries(bm_opf_high_vs_low PRIVATE benchmark absl::span)
target_include_directories(bm_opf_high_vs_low PRIVATE ../src/sfizz ../src/external)

add_executable(bm_write BM_writeInterleaved.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_write PRIVATE benchmark absl::span)
target_include_directories(bm_write PRIVATE ../src/sfizz ../src/external)

add_executable(bm_read BM_readInterleaved.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_read PRIVATE benchmark absl::span)
target_include_directories(bm_read PRIVATE ../src/sfizz ../src/external)

add_executable(bm_fill BM_fill.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_fill PRIVATE benchmark absl::span)
target_include_directories(bm_fill PRIVATE ../src/sfizz ../src/external)

add_executable(bm_mathfuns BM_mathfuns.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_mathfuns PRIVATE benchmark absl::span)
target_include_directories(bm_mathfuns PRIVATE ../src/sfizz ../src/external)

add_executable(bm_gain BM_gain.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_gain PRIVATE benchmark absl::span)
target_include_directories(bm_gain PRIVATE ../src/sfizz ../src/external)

add_executable(bm_divide BM_divide.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_divide PRIVATE benchmark absl::span)
target_include_directories(bm_divide PRIVATE ../src/sfizz ../src/external)

add_executable(bm_looping BM_looping.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_looping PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_looping PRIVATE ../src/sfizz ../src/external)

add_executable(bm_saturating BM_saturating.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_saturating PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_saturating PRIVATE ../src/sfizz ../src/external)

add_executable(bm_ramp BM_ramp.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_ramp PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_ramp PRIVATE ../src/sfizz ../src/external)

add_executable(bm_ADSR BM_ADSR.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_ADSR PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_ADSR PRIVATE ../src/sfizz ../src/external)

add_executable(bm_add BM_add.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_add PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_add PRIVATE ../src/sfizz ../src/external)

add_executable(bm_multiplyAdd BM_multiplyAdd.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_multiplyAdd PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_multiplyAdd PRIVATE ../src/sfizz ../src/external)

add_executable(bm_subtract BM_subtract.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_subtract PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_subtract PRIVATE ../src/sfizz ../src/external)

add_executable(bm_copy BM_copy.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_copy PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_copy PRIVATE ../src/sfizz ../src/external)

add_executable(bm_pan BM_pan.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_pan PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_pan PRIVATE ../src/sfizz ../src/external)

add_executable(bm_mean BM_mean.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_mean PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_mean PRIVATE ../src/sfizz ../src/external)

add_executable(bm_meanSquared BM_meanSquared.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_meanSquared PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_meanSquared PRIVATE ../src/sfizz ../src/external)

add_executable(bm_cumsum BM_cumsum.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_cumsum PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_cumsum PRIVATE ../src/sfizz ../src/external)

add_executable(bm_diff BM_diff.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_diff PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_diff PRIVATE ../src/sfizz ../src/external)

add_executable(bm_interpolationCast BM_interpolationCast.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_interpolationCast PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_interpolationCast PRIVATE ../src/sfizz ../src/external)

add_executable(bm_pointerIterationOrOffsets BM_pointerIterationOrOffsets.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_pointerIterationOrOffsets PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_pointerIterationOrOffsets PRIVATE ../src/sfizz ../src/external)

add_executable(bm_resample BM_resample.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_resample PRIVATE benchmark absl::span absl::algorithm)
sfizz_link_libsndfile(bm_resample)
sfizz_link_libsamplerate(bm_resample)
target_include_directories(bm_resample PRIVATE ../src/sfizz ../src/external)

add_executable(bm_envelopes BM_envelopes.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_envelopes PRIVATE benchmark absl::span absl::algorithm)
target_include_directories(bm_envelopes PRIVATE ../src/sfizz ../src/external)

add_executable(bm_wavfile BM_wavfile.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_wavfile PRIVATE benchmark absl::span absl::algorithm)
sfizz_link_libsndfile(bm_wavfile)
target_include_directories(bm_wavfile PRIVATE ../src/sfizz ../src/external)

add_executable(bm_flacfile BM_flacfile.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_flacfile PRIVATE benchmark absl::span absl::algorithm)
sfizz_link_libsndfile(bm_flacfile)
target_include_directories(bm_flacfile PRIVATE ../src/sfizz ../src/external)

add_executable(bm_readChunk BM_readChunk.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_readChunk PRIVATE benchmark absl::span absl::algorithm)
sfizz_link_libsndfile(bm_readChunk)
target_include_directories(bm_readChunk PRIVATE ../src/sfizz ../src/external)

add_executable(bm_resampleChunk BM_resampleChunk.cpp ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_resampleChunk PRIVATE benchmark absl::span absl::algorithm)
sfizz_link_libsndfile(bm_resampleChunk)
target_include_directories(bm_resampleChunk PRIVATE ../src/sfizz ../src/external)

add_custom_target(sfizz_benchmarks)
add_dependencies(sfizz_benchmarks
	bm_opf_high_vs_low
	bm_write
	bm_pointerIterationOrOffsets
	bm_read
	bm_mean
	bm_meanSquared
	bm_fill
	bm_cumsum
	bm_diff
	bm_interpolationCast
	bm_mathfuns
	bm_gain
	bm_divide
	bm_looping
	bm_saturating
	bm_ramp
	bm_ADSR
	bm_add
	bm_pan
	bm_subtract
	bm_multiplyAdd
	bm_readChunk
	bm_resample
	bm_resampleChunk
	bm_envelopes
	bm_wavfile
	bm_flacfile
)

file(COPY "sample1.wav" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample2.wav" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample3.wav" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample1.flac" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample2.flac" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample3.flac" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
