project(sfizz)

add_library(sfizz_lv2 SHARED sfizz.c)
set_target_properties(sfizz_lv2 PROPERTIES PREFIX "")
set_target_properties(sfizz_lv2 PROPERTIES OUTPUT_NAME "sfizz")
target_link_libraries(sfizz_lv2 sfizz::sfizz)