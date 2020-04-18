project (sfizz)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JACK "jack" REQUIRED)

add_executable (sfizz_jack jack_client.cpp)
target_include_directories (sfizz_jack PRIVATE ${JACK_INCLUDE_DIRS})
target_link_libraries (sfizz_jack PRIVATE sfizz::sfizz jack absl::flags_parse ${JACK_LIBRARIES})
sfizz_enable_lto_if_needed (sfizz_jack)
install (TARGETS sfizz_jack DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT "jack" OPTIONAL)
