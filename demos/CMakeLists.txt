find_package(PkgConfig)
if(PKGCONFIG_FOUND)
    pkg_check_modules(JACK "jack")
endif()
find_package(Qt5 COMPONENTS Widgets)

if(TARGET Qt5::Widgets)
    if(JACK_FOUND)
        add_executable(sfizz_demo_filters DemoFilters.cpp)
        target_include_directories(sfizz_demo_filters PRIVATE ${JACK_INCLUDE_DIRS})
        target_link_libraries(sfizz_demo_filters PRIVATE sfizz::sfizz Qt5::Widgets ${JACK_LIBRARIES})
        set_target_properties(sfizz_demo_filters PROPERTIES AUTOUIC ON)

        add_executable(sfizz_demo_smooth DemoSmooth.cpp)
        target_include_directories(sfizz_demo_smooth PRIVATE ${JACK_INCLUDE_DIRS})
        target_link_libraries(sfizz_demo_smooth PRIVATE sfizz::sfizz Qt5::Widgets ${JACK_LIBRARIES})
        set_target_properties(sfizz_demo_smooth PROPERTIES AUTOUIC ON)

        add_executable(sfizz_demo_stereo DemoStereo.cpp)
        target_include_directories(sfizz_demo_stereo PRIVATE ${JACK_INCLUDE_DIRS})
        target_link_libraries(sfizz_demo_stereo PRIVATE sfizz::sfizz Qt5::Widgets ${JACK_LIBRARIES})
        set_target_properties(sfizz_demo_stereo PROPERTIES AUTOUIC ON)

        add_executable(sfizz_demo_wavetables DemoWavetables.cpp)
        target_include_directories(sfizz_demo_wavetables PRIVATE ${JACK_INCLUDE_DIRS})
        target_link_libraries(sfizz_demo_wavetables PRIVATE sfizz::sfizz Qt5::Widgets ${JACK_LIBRARIES})
        set_target_properties(sfizz_demo_wavetables PROPERTIES AUTOUIC ON)
    endif()

    add_executable(sfizz_demo_parser DemoParser.cpp)
    target_link_libraries(sfizz_demo_parser PRIVATE sfizz_parser Qt5::Widgets)
    set_target_properties(sfizz_demo_parser PROPERTIES AUTOUIC ON)

    add_executable(sfizz_demo_stretch_tuning DemoStretchTuning.cpp)
    target_link_libraries(sfizz_demo_stretch_tuning PRIVATE sfizz::sfizz Qt5::Widgets)
    set_target_properties(sfizz_demo_stretch_tuning PROPERTIES AUTOUIC ON)
endif()

add_executable(eq_apply EQ.cpp)
target_link_libraries(eq_apply PRIVATE sfizz::sfizz sfizz::sndfile sfizz::cxxopts sfizz::filesystem)

add_executable(filter_apply Filter.cpp)
target_link_libraries(filter_apply PRIVATE sfizz::sfizz sfizz::sndfile sfizz::cxxopts sfizz::filesystem)

add_executable(sfizz_plot_curve PlotCurve.cpp)
target_link_libraries(sfizz_plot_curve PRIVATE sfizz::sfizz)

add_executable(sfizz_plot_wavetables PlotWavetables.cpp)
target_link_libraries(sfizz_plot_wavetables PRIVATE sfizz::sfizz)

add_executable(sfizz_plot_lfo PlotLFO.cpp)
target_link_libraries(sfizz_plot_lfo PRIVATE sfizz::sfizz sfizz::sndfile sfizz::cxxopts)

add_executable(sfizz_file_instrument FileInstrument.cpp)
target_link_libraries(sfizz_file_instrument PRIVATE sfizz::sfizz sfizz::sndfile)

add_executable(sfizz_file_wavetable FileWavetable.cpp)
target_link_libraries(sfizz_file_wavetable PRIVATE sfizz::sfizz)

add_executable(sfizz_tuning Tuning.cpp)
target_link_libraries(sfizz_tuning PRIVATE sfizz::sfizz sfizz::cxxopts)
