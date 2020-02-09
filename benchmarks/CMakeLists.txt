project(sfizz)

# Check SIMD
include (SfizzSIMDSourceFilesCheck)
set(BENCHMARK_SIMD_SOURCES ${SFIZZ_SIMD_SOURCES})
list(TRANSFORM BENCHMARK_SIMD_SOURCES PREPEND "../src/")
find_package(benchmark CONFIG REQUIRED)

# Check libsamplerate
find_library(SAMPLERATE_LIBRARY "samplerate")
find_path(SAMPLERATE_INCLUDE_DIR "samplerate.h")
message(STATUS "Checking samplerate library: ${SAMPLERATE_LIBRARY}")
message(STATUS "Checking samplerate includes: ${SAMPLERATE_INCLUDE_DIR}")
if(SAMPLERATE_LIBRARY AND SAMPLERATE_INCLUDE_DIR)
    add_library(sfizz-samplerate INTERFACE)
    target_include_directories(sfizz-samplerate INTERFACE "${SAMPLERATE_INCLUDE_DIR}")
    target_link_libraries(sfizz-samplerate INTERFACE "${SAMPLERATE_LIBRARY}")
endif()

add_library(bm_simd STATIC ${BENCHMARK_SIMD_SOURCES})
target_link_libraries(bm_simd PRIVATE absl::span)
target_include_directories(bm_simd PRIVATE ../src/external)
add_library(bm_ftz STATIC ../src/sfizz/ScopedFTZ.cpp)

macro(sfizz_add_benchmark TARGET)
	add_executable("${TARGET}" ${ARGN})
	target_link_libraries("${TARGET}"
		PRIVATE absl::span absl::algorithm
		PRIVATE benchmark::benchmark benchmark::benchmark_main)
	target_include_directories("${TARGET}" PRIVATE ../src/sfizz ../src/external)
endmacro()

sfizz_add_benchmark(bm_opf_high_vs_low BM_OPF_high_vs_low.cpp)

sfizz_add_benchmark(bm_write BM_writeInterleaved.cpp)
target_link_libraries(bm_write PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_read BM_readInterleaved.cpp)
target_link_libraries(bm_read PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_fill BM_fill.cpp)
target_link_libraries(bm_fill PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_mathfuns BM_mathfuns.cpp)
target_link_libraries(bm_mathfuns PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_gain BM_gain.cpp)
target_link_libraries(bm_gain PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_divide BM_divide.cpp)
target_link_libraries(bm_divide PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_looping BM_looping.cpp)
target_link_libraries(bm_looping PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_saturating BM_saturating.cpp)
target_link_libraries(bm_saturating PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_ramp BM_ramp.cpp)
target_link_libraries(bm_ramp PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_ADSR BM_ADSR.cpp)
target_link_libraries(bm_ADSR PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_add BM_add.cpp)
target_link_libraries(bm_add PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_multiplyAdd BM_multiplyAdd.cpp)
target_link_libraries(bm_multiplyAdd PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_subtract BM_subtract.cpp)
target_link_libraries(bm_subtract PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_copy BM_copy.cpp)
target_link_libraries(bm_copy PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_pan BM_pan.cpp)
target_link_libraries(bm_pan PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_mean BM_mean.cpp)
target_link_libraries(bm_mean PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_meanSquared BM_meanSquared.cpp)
target_link_libraries(bm_meanSquared PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_cumsum BM_cumsum.cpp)
target_link_libraries(bm_cumsum PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_diff BM_diff.cpp)
target_link_libraries(bm_diff PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_interpolationCast BM_interpolationCast.cpp)
target_link_libraries(bm_interpolationCast PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_pointerIterationOrOffsets BM_pointerIterationOrOffsets.cpp)
target_link_libraries(bm_pointerIterationOrOffsets PRIVATE bm_simd bm_ftz)

if (TARGET sfizz-samplerate)
	sfizz_add_benchmark(bm_resample BM_resample.cpp ${BENCHMARK_SIMD_SOURCES})
	target_link_libraries(bm_resample PRIVATE sfizz-samplerate sfizz-sndfile)
endif()

sfizz_add_benchmark(bm_envelopes BM_envelopes.cpp)
target_link_libraries(bm_envelopes PRIVATE bm_simd bm_ftz)

sfizz_add_benchmark(bm_wavfile BM_wavfile.cpp)
target_link_libraries(bm_wavfile PRIVATE sfizz-sndfile bm_simd bm_ftz)

sfizz_add_benchmark(bm_flacfile BM_flacfile.cpp)
target_link_libraries(bm_flacfile PRIVATE sfizz-sndfile bm_simd bm_ftz)

sfizz_add_benchmark(bm_readChunk BM_readChunk.cpp)
target_link_libraries(bm_readChunk PRIVATE sfizz-sndfile bm_simd bm_ftz)

sfizz_add_benchmark(bm_resampleChunk BM_resampleChunk.cpp)
target_link_libraries(bm_resampleChunk PRIVATE sfizz-sndfile bm_simd bm_ftz)

sfizz_add_benchmark(bm_filterModulation BM_filterModulation.cpp ../src/sfizz/SfzFilter.cpp)
target_link_libraries(bm_filterModulation PRIVATE sfizz-sndfile bm_simd bm_ftz)

sfizz_add_benchmark(bm_filterStereoMono BM_filterStereoMono.cpp ../src/sfizz/SfzFilter.cpp)
target_link_libraries(bm_filterStereoMono PRIVATE sfizz-sndfile bm_simd bm_ftz)

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

if (TARGET bm_resample)
	add_dependencies(sfizz_benchmarks bm_resample)
endif()

file(COPY "sample1.wav" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample2.wav" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample3.wav" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample1.flac" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample2.flac" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
file(COPY "sample3.flac" DESTINATION ${CMAKE_BINARY_DIR}/benchmarks)
