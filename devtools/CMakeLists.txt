###############################
# Developer tools

find_package(PkgConfig)
if(PKGCONFIG_FOUND)
pkg_check_modules(JACK "jack")
endif()
find_package(Qt5 COMPONENTS Widgets)

if(JACK_FOUND AND TARGET Qt5::Widgets)
    add_executable(sfizz_capture_eg CaptureEG.h CaptureEG.cpp)
    target_include_directories(sfizz_capture_eg PRIVATE . ${JACK_INCLUDE_DIRS})
    target_link_libraries(sfizz_capture_eg PRIVATE sfizz-sndfile Qt5::Widgets ${JACK_LIBRARIES})
    set_target_properties(sfizz_capture_eg PROPERTIES AUTOUIC ON)
endif()

add_executable(sfizz_preprocessor Preprocessor.cpp)
target_link_libraries(sfizz_preprocessor sfizz_parser)
