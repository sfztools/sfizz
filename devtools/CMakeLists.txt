###############################
# Developer tools

if(TARGET sfizz::jack AND TARGET Qt5::Widgets)
    add_executable(sfizz_capture_eg CaptureEG.h CaptureEG.cpp)
    target_include_directories(sfizz_capture_eg PRIVATE .)
    target_link_libraries(sfizz_capture_eg PRIVATE sfizz::sndfile Qt5::Widgets sfizz::jack)
    set_target_properties(sfizz_capture_eg PROPERTIES AUTOUIC ON)
endif()

add_executable(sfizz_preprocessor Preprocessor.cpp)
target_link_libraries(sfizz_preprocessor PRIVATE sfizz::parser sfizz::pugixml sfizz::cxxopts)
