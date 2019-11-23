project(sfizz)

add_library(sfizz_lv2 SHARED sfizz.c)
set_target_properties(sfizz_lv2 PROPERTIES PREFIX "")
set_target_properties(sfizz_lv2 PROPERTIES OUTPUT_NAME "sfizz")
target_link_libraries(sfizz_lv2 sfizz::sfizz)

add_custom_command( TARGET sfizz_lv2 POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                            ${CMAKE_SOURCE_DIR}/lv2/sfizz.ttl
                            ${CMAKE_CURRENT_BINARY_DIR}/sfizz.ttl)

add_custom_command( TARGET sfizz_lv2 POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy
                            ${CMAKE_SOURCE_DIR}/lv2/manifest.ttl
                            ${CMAKE_CURRENT_BINARY_DIR}/manifest.ttl)

# TODO: install target for the LV2 plugin