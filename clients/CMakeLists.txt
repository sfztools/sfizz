project (sfizz)

find_package(PkgConfig REQUIRED)
pkg_check_modules(JACK "jack" REQUIRED)

add_executable (sfizz_jack jack_client.cpp)
target_include_directories (sfizz_jack PRIVATE ${JACK_INCLUDE_DIRS})
target_link_libraries (sfizz_jack PRIVATE sfizz::sfizz jack absl::flags_parse ${JACK_LIBRARIES})
sfizz_enable_lto_if_needed (sfizz_jack)
install (TARGETS sfizz_jack DESTINATION ${CMAKE_INSTALL_BINDIR}
COMPONENT "jack" OPTIONAL)

add_library(fmidi-fmt INTERFACE)
target_include_directories(fmidi-fmt INTERFACE "external/fmt/include")
target_compile_definitions(fmidi-fmt INTERFACE "FMT_HEADER_ONLY=1")
add_subdirectory(external/fmidi EXCLUDE_FROM_ALL)

add_executable(sfizz_render sfizz_render.cpp)
target_link_libraries(sfizz_render sfizz::sfizz fmidi)
sfizz_enable_lto_if_needed (sfizz_render)
install (TARGETS sfizz_render DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
