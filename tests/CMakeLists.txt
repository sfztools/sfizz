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

find_package(ZLIB REQUIRED)
add_library(cnpy cnpy/cnpy.cpp)
target_link_libraries(cnpy PUBLIC ZLIB::ZLIB)
target_include_directories(cnpy PUBLIC cnpy)

add_executable(sfizz_tests ${SFIZZ_TEST_SOURCES})
target_link_libraries(sfizz_tests PRIVATE sfizz::sfizz cnpy)

# target_link_libraries(sfizz_tests PRIVATE absl::strings absl::str_format absl::flat_hash_map cnpy absl::span absl::algorithm)

file(COPY "." DESTINATION ${CMAKE_BINARY_DIR}/tests)
