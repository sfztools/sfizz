project(sfizz)

###############################
add_executable(sfzprint sfzprint.cpp)
target_link_libraries(sfzprint sfizz::parser absl::flags_parse)
sfizz_enable_lto_if_needed(sfzprint)
# install(TARGETS sfzprint DESTINATION . OPTIONAL)

add_executable(sfizz_jack jack_client.cpp)
target_link_libraries(sfizz_jack sfizz::sfizz jack absl::flags_parse)
sfizz_enable_lto_if_needed(sfizz_jack)
# install(TARGETS sfizz_jack DESTINATION . OPTIONAL)
