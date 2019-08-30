project(sfizz)

###############################
add_executable(sfzprint sfzprint.cpp)
target_link_libraries(sfzprint sfizz::parser absl::flags_parse)

###############################
# Basic command line program
add_executable(sfizz_jack jack_client.cpp)
target_link_libraries(sfizz_jack sfizz::sfizz jack absl::flags_parse)
