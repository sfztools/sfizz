###############################
# Test application

project(sfizz)

set(SFIZZ_TEST_SOURCES
    RegexT.cpp
    HelpersT.cpp
    HelpersT.cpp
    AudioBufferT.cpp
    RangeT.cpp
    OpcodeT.cpp
    BufferT.cpp
    StereoBufferT.cpp
    SIMDHelpersT.cpp
    FilesT.cpp
    OnePoleFilterT.cpp
    RegionActivationT.cpp
    RegionCrossfadesT.cpp
    ADSREnvelopeT.cpp
    LinearEnvelopeT.cpp
    MainT.cpp
    RegionTriggersT.cpp
)

add_executable(sfizz_tests ${SFIZZ_TEST_SOURCES})
target_link_libraries(sfizz_tests PRIVATE sfizz)

# Per OS properties
if(UNIX)
    target_link_libraries(sfizz_tests PRIVATE stdc++fs atomic)
endif(UNIX)
target_link_libraries(sfizz_tests PRIVATE Catch2::Catch2 absl::strings absl::str_format absl::flat_hash_map sndfile readerwriterqueue cnpy-static absl::span absl::algorithm)
target_include_directories(sfizz_tests SYSTEM PRIVATE sources)

file(COPY "." DESTINATION ${CMAKE_BINARY_DIR}/tests)