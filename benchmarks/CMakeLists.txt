project(sfizz)

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