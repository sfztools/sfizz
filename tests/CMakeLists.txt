###############################
# Test application

project(sfizz)

set(SFIZZ_TEST_SOURCES
    RegionT.cpp
    ParsingT.cpp
    HelpersT.cpp
    HelpersT.cpp
    AudioBufferT.cpp
    EGDescriptionT.cpp
    RangeT.cpp
    OpcodeT.cpp
    BufferT.cpp
    SIMDHelpersT.cpp
    FilesT.cpp
    MidiStateT.cpp
    OnePoleFilterT.cpp
    RegionActivationT.cpp
    RegionValueComputationsT.cpp
    ADSREnvelopeT.cpp
    EventEnvelopesT.cpp
    MainT.cpp
    SynthT.cpp
    RegionTriggersT.cpp
)

add_executable(sfizz_tests ${SFIZZ_TEST_SOURCES})
target_link_libraries(sfizz_tests PRIVATE sfizz::sfizz)
sfizz_enable_lto_if_needed(sfizz_tests)
# target_link_libraries(sfizz_tests PRIVATE absl::strings absl::str_format absl::flat_hash_map cnpy absl::span absl::algorithm)

file(COPY "." DESTINATION ${CMAKE_BINARY_DIR}/tests)
