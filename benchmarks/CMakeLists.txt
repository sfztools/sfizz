project(sfizz)

# Check SIMD
include (SfizzSIMDSourceFilesCheck)
set(BENCHMARK_SIMD_SOURCES ${SFIZZ_SIMD_SOURCES})
list(TRANSFORM BENCHMARK_SIMD_SOURCES PREPEND "../src/")
find_package(benchmark CONFIG REQUIRED)

add_library(bm_simd STATIC ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_simd PRIVATE absl::span)
target_include_directories(bm_simd PRIVATE ../src/external)
add_library(bm_ftz STATIC ../src/sfizz/ScopedFTZ.cpp)

add_executable(bm_opf_high_vs_low BM_OPF_high_vs_low.cpp)
target_link_libraries(bm_opf_high_vs_low PRIVATE absl::span benchmark::benchmark benchmark::benchmark_main)
target_include_directories(bm_opf_high_vs_low PRIVATE ../src/sfizz ../src/external)

add_executable(bm_write BM_writeInterleaved.cpp)
target_link_libraries(bm_write PRIVATE absl::span benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_write PRIVATE ../src/sfizz ../src/external)

add_executable(bm_read BM_readInterleaved.cpp)
target_link_libraries(bm_read PRIVATE absl::span benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_read PRIVATE ../src/sfizz ../src/external)

add_executable(bm_fill BM_fill.cpp)
target_link_libraries(bm_fill PRIVATE absl::span benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_fill PRIVATE ../src/sfizz ../src/external)

add_executable(bm_mathfuns BM_mathfuns.cpp)
target_link_libraries(bm_mathfuns PRIVATE absl::span benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_mathfuns PRIVATE ../src/sfizz ../src/external)

add_executable(bm_gain BM_gain.cpp)
target_link_libraries(bm_gain PRIVATE absl::span benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_gain PRIVATE ../src/sfizz ../src/external)

add_executable(bm_divide BM_divide.cpp)
target_link_libraries(bm_divide PRIVATE absl::span benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_divide PRIVATE ../src/sfizz ../src/external)

add_executable(bm_looping BM_looping.cpp)
target_link_libraries(bm_looping PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_looping PRIVATE ../src/sfizz ../src/external)

add_executable(bm_saturating BM_saturating.cpp)
target_link_libraries(bm_saturating PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_saturating PRIVATE ../src/sfizz ../src/external)

add_executable(bm_ramp BM_ramp.cpp)
target_link_libraries(bm_ramp PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_ramp PRIVATE ../src/sfizz ../src/external)

add_executable(bm_ADSR BM_ADSR.cpp)
target_link_libraries(bm_ADSR PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_ADSR PRIVATE ../src/sfizz ../src/external)

add_executable(bm_add BM_add.cpp)
target_link_libraries(bm_add PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_add PRIVATE ../src/sfizz ../src/external)

add_executable(bm_multiplyAdd BM_multiplyAdd.cpp)
target_link_libraries(bm_multiplyAdd PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_multiplyAdd PRIVATE ../src/sfizz ../src/external)

add_executable(bm_subtract BM_subtract.cpp)
target_link_libraries(bm_subtract PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_subtract PRIVATE ../src/sfizz ../src/external)

add_executable(bm_copy BM_copy.cpp)
target_link_libraries(bm_copy PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_copy PRIVATE ../src/sfizz ../src/external)

add_executable(bm_pan BM_pan.cpp)
target_link_libraries(bm_pan PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_pan PRIVATE ../src/sfizz ../src/external)

add_executable(bm_mean BM_mean.cpp)
target_link_libraries(bm_mean PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_mean PRIVATE ../src/sfizz ../src/external)

add_executable(bm_meanSquared BM_meanSquared.cpp)
target_link_libraries(bm_meanSquared PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_meanSquared PRIVATE ../src/sfizz ../src/external)

add_executable(bm_cumsum BM_cumsum.cpp)
target_link_libraries(bm_cumsum PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_cumsum PRIVATE ../src/sfizz ../src/external)

add_executable(bm_diff BM_diff.cpp)
target_link_libraries(bm_diff PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_diff PRIVATE ../src/sfizz ../src/external)

add_executable(bm_interpolationCast BM_interpolationCast.cpp)
target_link_libraries(bm_interpolationCast PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_interpolationCast PRIVATE ../src/sfizz ../src/external)

add_executable(bm_pointerIterationOrOffsets BM_pointerIterationOrOffsets.cpp)
target_link_libraries(bm_pointerIterationOrOffsets PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_pointerIterationOrOffsets PRIVATE ../src/sfizz ../src/external)

if (NOT WIN32)
	add_executable(bm_resample BM_resample.cpp ${BENCHMARK_SIMD_SOURCES})
	target_link_libraries(bm_resample PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main samplerate sfizz-sndfile)
	target_include_directories(bm_resample PRIVATE ../src/sfizz ../src/external)
endif()

add_executable(bm_envelopes BM_envelopes.cpp)
target_link_libraries(bm_envelopes PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main bm_simd bm_ftz)
target_include_directories(bm_envelopes PRIVATE ../src/sfizz ../src/external)

add_executable(bm_wavfile BM_wavfile.cpp)
target_link_libraries(bm_wavfile PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main sfizz-sndfile bm_simd bm_ftz)
target_include_directories(bm_wavfile PRIVATE ../src/sfizz ../src/external)

add_executable(bm_flacfile BM_flacfile.cpp)
target_link_libraries(bm_flacfile PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main sfizz-sndfile bm_simd bm_ftz)
target_include_directories(bm_flacfile PRIVATE ../src/sfizz ../src/external)

add_executable(bm_readChunk BM_readChunk.cpp)
target_link_libraries(bm_readChunk PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main sfizz-sndfile bm_simd bm_ftz)
target_include_directories(bm_readChunk PRIVATE ../src/sfizz ../src/external)

add_executable(bm_resampleChunk BM_resampleChunk.cpp)
target_link_libraries(bm_resampleChunk PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main sfizz-sndfile bm_simd bm_ftz)
target_include_directories(bm_resampleChunk PRIVATE ../src/sfizz ../src/external)

add_executable(bm_filterModulation BM_filterModulation.cpp ${BENCHMARK_SIMD_SOURCES} ../src/sfizz/SfzFilter.cpp)
target_link_libraries(bm_filterModulation PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main sfizz-sndfile bm_ftz)
target_include_directories(bm_filterModulation PRIVATE ../src/sfizz ../src/external)

add_executable(bm_filterStereoMono BM_filterStereoMono.cpp ${BENCHMARK_SIMD_SOURCES} ../src/sfizz/SfzFilter.cpp)
target_link_libraries(bm_filterStereoMono PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main sfizz-sndfile bm_ftz)
target_include_directories(bm_filterStereoMono PRIVATE ../src/sfizz ../src/external)

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
	bm_resampleChunk
	bm_envelopes
	bm_wavfile
	bm_flacfile
	bm_filterModulation
	bm_filterStereoMono
)

if (NOT WIN32)
	add_dependencies(sfizz_benchmarks bm_resample)
endif()

file(COPY "sample1.wav" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample2.wav" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample3.wav" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample1.flac" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample2.flac" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample3.flac" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
