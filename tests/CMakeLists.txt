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
    SfzHelpersT.cpp
    RegionActivationT.cpp
    RegionValueComputationsT.cpp
    # If we're tweaking the curves this kind of tests does not make sense
    # Use integration tests with comparison curves
    # ADSREnvelopeT.cpp
    EventEnvelopesT.cpp
    MainT.cpp
    SynthT.cpp
    CurveT.cpp
    RegionTriggersT.cpp
    FloatHelpersT.cpp
    WavetablesT.cpp
    SemaphoreT.cpp
)

add_executable(sfizz_tests ${SFIZZ_TEST_SOURCES})
target_link_libraries(sfizz_tests PRIVATE sfizz::sfizz)
sfizz_enable_lto_if_needed(sfizz_tests)
# target_link_libraries(sfizz_tests PRIVATE absl::strings absl::str_format absl::flat_hash_map cnpy absl::span absl::algorithm)

find_package(PkgConfig)
if(PKGCONFIG_FOUND)
pkg_check_modules(JACK "jack")
endif()
find_package(Qt5 COMPONENTS Widgets)

if(JACK_FOUND AND TARGET Qt5::Widgets)
    add_executable(sfizz_demo_filters DemoFilters.cpp)
    target_include_directories(sfizz_demo_filters PRIVATE ${JACK_INCLUDE_DIRS})
    target_link_libraries(sfizz_demo_filters PRIVATE sfizz::sfizz Qt5::Widgets ${JACK_LIBRARIES})
    set_target_properties(sfizz_demo_filters PROPERTIES AUTOUIC ON)

    add_executable(sfizz_demo_stereo DemoStereo.cpp)
    target_include_directories(sfizz_demo_stereo PRIVATE ${JACK_INCLUDE_DIRS})
    target_link_libraries(sfizz_demo_stereo PRIVATE sfizz::sfizz Qt5::Widgets ${JACK_LIBRARIES})
    set_target_properties(sfizz_demo_stereo PROPERTIES AUTOUIC ON)

    add_executable(sfizz_demo_wavetables DemoWavetables.cpp)
    target_include_directories(sfizz_demo_wavetables PRIVATE ${JACK_INCLUDE_DIRS})
    target_link_libraries(sfizz_demo_wavetables PRIVATE sfizz::sfizz Qt5::Widgets ${JACK_LIBRARIES})
    set_target_properties(sfizz_demo_wavetables PROPERTIES AUTOUIC ON)

    add_executable(sfizz_demo_parser DemoParser.cpp)
    target_link_libraries(sfizz_demo_parser PRIVATE sfizz_parser Qt5::Widgets)
    set_target_properties(sfizz_demo_parser PROPERTIES AUTOUIC ON)
endif()

add_executable(eq_apply EQ.cpp)
target_link_libraries(eq_apply PRIVATE sfizz::sfizz)

add_executable(filter_apply Filter.cpp)
target_link_libraries(filter_apply PRIVATE sfizz::sfizz)

add_executable(sfizz_plot_curve PlotCurve.cpp)
target_link_libraries(sfizz_plot_curve PRIVATE sfizz::sfizz)

add_executable(sfizz_plot_wavetables PlotWavetables.cpp)
target_link_libraries(sfizz_plot_wavetables PRIVATE sfizz::sfizz)

file(COPY "." DESTINATION ${CMAKE_BINARY_DIR}/tests)
