if(SFIZZ_JACK)
    add_executable(sfizz_jack MidiHelpers.h jack_client.cpp)
    target_link_libraries(sfizz_jack PRIVATE sfizz::sfizz sfizz::jack sfizz::spin_mutex absl::flags_parse)
    sfizz_enable_lto_if_needed(sfizz_jack)
    install(TARGETS sfizz_jack DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT "jack" OPTIONAL)
endif()

if(SFIZZ_RENDER)
    add_executable(sfizz_render MidiHelpers.h sfizz_render.cpp)
    target_link_libraries(sfizz_render PRIVATE sfizz::internal sfizz::fmidi sfizz::cxxopts st_audiofile_formats)
    sfizz_enable_lto_if_needed(sfizz_render)
    install(TARGETS sfizz_render DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT "render" OPTIONAL)
endif()
