project (sfizz)

add_library (sfizz_lv2 SHARED sfizz.c)
set_target_properties (sfizz_lv2 PROPERTIES PREFIX "")
set_target_properties (sfizz_lv2 PROPERTIES OUTPUT_NAME "sfizz")
target_link_libraries (sfizz_lv2 sfizz::sfizz)

add_custom_command (TARGET sfizz_lv2 POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                            ${CMAKE_SOURCE_DIR}/lv2/sfizz.ttl
                            ${CMAKE_CURRENT_BINARY_DIR}/sfizz.ttl)

add_custom_command (TARGET sfizz_lv2 POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                            ${CMAKE_SOURCE_DIR}/lv2/manifest.ttl
                            ${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl)
if (SFIZZ_USE_LTO)
    set_property (TARGET sfizz_lv2 PROPERTY INTERPROCEDURAL_OPTIMIZATION True)
endif()

if (UNIX)
    if (NOT LV2_INSTALL_PREFIX)
        set (LV2_INSTALL_PREFIX ${CMAKE_INSTALL_LIBDIR}/lv2/sfizz.lv2)
    endif()
    install (TARGETS sfizz_lv2
        ARCHIVE DESTINATION ${LV2_INSTALL_PREFIX}
        LIBRARY DESTINATION ${LV2_INSTALL_PREFIX})
    install (FILES ${CMAKE_BINARY_DIR}/lv2/sfizz.ttl DESTINATION ${LV2_INSTALL_PREFIX})
    install (FILES ${CMAKE_BINARY_DIR}/lv2/manifest.ttl DESTINATION ${LV2_INSTALL_PREFIX})
endif()
