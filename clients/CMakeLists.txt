project (sfizz)

if (SFIZZ_JACK)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JACK "jack" REQUIRED)
    link_directories (${JACK_LIBRARY_DIRS})

    add_executable (sfizz_jack MidiHelpers.h jack_client.cpp)
    target_include_directories (sfizz_jack PRIVATE ${JACK_INCLUDE_DIRS})
    target_link_libraries (sfizz_jack PRIVATE sfizz::sfizz absl::flags_parse sfizz-sndfile ${JACK_LIBRARIES})
    sfizz_enable_lto_if_needed (sfizz_jack)
    install (TARGETS sfizz_jack DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT "jack" OPTIONAL)
endif()

if (SFIZZ_RENDER)
    add_library(sfizz-fmidi STATIC
        "external/fmidi/sources/fmidi/fmidi.h"
        "external/fmidi/sources/fmidi/fmidi_mini.cpp")
    target_include_directories(sfizz-fmidi PUBLIC "external/fmidi/sources")
    target_compile_definitions(sfizz-fmidi PUBLIC "FMIDI_STATIC=1" "FMIDI_DISABLE_DESCRIBE_API=1")

    add_executable(sfizz_render MidiHelpers.h sfizz_render.cpp)
    target_link_libraries(sfizz_render PRIVATE sfizz::sfizz sfizz-fmidi sfizz-sndfile)
    sfizz_enable_lto_if_needed (sfizz_render)
    install (TARGETS sfizz_render DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT "render" OPTIONAL)
endif()
