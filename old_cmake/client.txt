project(sfizz)

###############################
add_executable(sfzprint sfzprint.cpp)
target_link_libraries(sfzprint sfizz::parser absl::flags_parse)
install(TARGETS sfzprint DESTINATION . OPTIONAL)

###############################
# Basic command line program
if(APPLE)
    find_path(JACK_INCLUDE_DIR NAMES jack.h PATHS "/usr/local/include/jack")
    find_library(JACK_LIBRARY  NAMES jack   PATHS "/usr/local/lib")
    message(STATUS "include dir: " ${JACK_INCLUDE_DIR} " lib: " ${JACK_LIBRARY})
    add_library(jack SHARED IMPORTED GLOBAL)
    target_include_directories(jack INTERFACE ${JACK_INCLUDE_DIR})
    target_link_libraries(jack INTERFACE ${JACK_LIBRARY})
endif()

add_executable(sfizz_jack jack_client.cpp)
target_link_libraries(sfizz_jack sfizz::sfizz jack absl::flags_parse)
install(TARGETS sfizz_jack DESTINATION . OPTIONAL)
