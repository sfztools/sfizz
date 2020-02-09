project(sfizz)

# Check SIMD
include (SfizzSIMDSourceFilesCheck)
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

macro(sfizz_add_benchmark TARGET)
	add_executable("${TARGET}" ${ARGN})
	target_link_libraries("${TARGET}" PRIVATE absl::span absl::algorithm benchmark::benchmark benchmark::benchmark_main sfizz_static)
	target_include_directories("${TARGET}" PRIVATE ../src/sfizz ../src/external)
endmacro()

sfizz_add_benchmark(bm_opf_high_vs_low BM_OPF_high_vs_low.cpp)
sfizz_add_benchmark(bm_write BM_writeInterleaved.cpp)
sfizz_add_benchmark(bm_read BM_readInterleaved.cpp)
sfizz_add_benchmark(bm_fill BM_fill.cpp)
sfizz_add_benchmark(bm_mathfuns BM_mathfuns.cpp)
sfizz_add_benchmark(bm_gain BM_gain.cpp)
sfizz_add_benchmark(bm_divide BM_divide.cpp)
sfizz_add_benchmark(bm_looping BM_looping.cpp)
sfizz_add_benchmark(bm_saturating BM_saturating.cpp)
sfizz_add_benchmark(bm_ramp BM_ramp.cpp)
sfizz_add_benchmark(bm_ADSR BM_ADSR.cpp)
sfizz_add_benchmark(bm_add BM_add.cpp)
sfizz_add_benchmark(bm_multiplyAdd BM_multiplyAdd.cpp)
sfizz_add_benchmark(bm_subtract BM_subtract.cpp)
sfizz_add_benchmark(bm_copy BM_copy.cpp)
sfizz_add_benchmark(bm_pan BM_pan.cpp)
sfizz_add_benchmark(bm_mean BM_mean.cpp)
sfizz_add_benchmark(bm_meanSquared BM_meanSquared.cpp)
sfizz_add_benchmark(bm_cumsum BM_cumsum.cpp)
sfizz_add_benchmark(bm_diff BM_diff.cpp)
sfizz_add_benchmark(bm_interpolationCast BM_interpolationCast.cpp)
sfizz_add_benchmark(bm_pointerIterationOrOffsets BM_pointerIterationOrOffsets.cpp)

if (TARGET sfizz-samplerate)
	sfizz_add_benchmark(bm_resample BM_resample.cpp)
	target_link_libraries(bm_resample PRIVATE sfizz-samplerate)
endif()

sfizz_add_benchmark(bm_envelopes BM_envelopes.cpp)
sfizz_add_benchmark(bm_wavfile BM_wavfile.cpp)
sfizz_add_benchmark(bm_flacfile BM_flacfile.cpp)
sfizz_add_benchmark(bm_readChunk BM_readChunk.cpp)
sfizz_add_benchmark(bm_resampleChunk BM_resampleChunk.cpp)

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
