string(TIMESTAMP MAN_TODAY "%Y-%m-%d")

if(SFIZZ_JACK)
    add_executable(sfizz_jack MidiHelpers.h jack_client.cpp)
    target_link_libraries(sfizz_jack PRIVATE
        absl::flags_parse
        sfizz::import
        sfizz::jack
        sfizz::sfizz
        sfizz::spin_mutex
    )
    sfizz_enable_lto_if_needed(sfizz_jack)
    configure_file(sfizz_jack.man.in sfizz_jack.man @ONLY)
    install(TARGETS sfizz_jack
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT "jack" OPTIONAL
    )
    install(FILES ${PROJECT_BINARY_DIR}/clients/sfizz_jack.man
        RENAME sfizz_jack.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        COMPONENT "jack" OPTIONAL
    )
endif()

if(SFIZZ_RENDER)
    add_executable(sfizz_render MidiHelpers.h sfizz_render.cpp)
    target_link_libraries(sfizz_render PRIVATE
        sfizz::cxxopts
        sfizz::fmidi
        sfizz::internal
        st_audiofile_formats
    )
    sfizz_enable_lto_if_needed(sfizz_render)
    configure_file(sfizz_render.man.in sfizz_render.man @ONLY)
    install(TARGETS sfizz_render
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT "render" OPTIONAL
    )
    install(FILES ${PROJECT_BINARY_DIR}/clients/sfizz_render.man
        RENAME sfizz_render.1
        DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
        COMPONENT "render" OPTIONAL
    )
endif()
