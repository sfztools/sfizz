###############################
# Test application

project(sfizz)

set(SFIZZ_TEST_SOURCES
    DirectRegionT.cpp
    RegionValuesT.cpp
    TestHelpers.h
    TestHelpers.cpp
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
    InterpolatorsT.cpp
    SmoothersT.cpp
    PolyphonyT.cpp
    RegionActivationT.cpp
    RegionValueComputationsT.cpp
    # If we're tweaking the curves this kind of tests does not make sense
    # Use integration tests with comparison curves
    # ADSREnvelopeT.cpp
    FlexEGT.cpp
    EventEnvelopesT.cpp
    MainT.cpp
    SynthT.cpp
    CurveT.cpp
    RegionTriggersT.cpp
    FloatHelpersT.cpp
    RandomHelpersT.cpp
    WavetablesT.cpp
    SemaphoreT.cpp
    SwapAndPopT.cpp
    TuningT.cpp
    ConcurrencyT.cpp
    ModulationsT.cpp
    LFOT.cpp
    MessagingT.cpp
    DataHelpers.h
    DataHelpers.cpp
)

add_executable(sfizz_tests ${SFIZZ_TEST_SOURCES})
target_link_libraries(sfizz_tests PRIVATE sfizz::sfizz sfizz-jsl)
sfizz_enable_lto_if_needed(sfizz_tests)
sfizz_enable_fast_math(sfizz_tests)
# target_link_libraries(sfizz_tests PRIVATE absl::strings absl::str_format absl::flat_hash_map cnpy absl::span absl::algorithm)

file(COPY "." DESTINATION ${CMAKE_BINARY_DIR}/tests)
