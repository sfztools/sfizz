project (sfizz)

if (APPLE)
	include_directories (SYSTEM /usr/local/opt/jack/include)
	link_directories (/usr/local/opt/jack/lib)
endif()

add_executable (sfizz_jack jack_client.cpp)
target_link_libraries (sfizz_jack sfizz::sfizz jack absl::flags_parse)
sfizz_enable_lto_if_needed (sfizz_jack)
install (TARGETS sfizz_jack DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
