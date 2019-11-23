cmake_minimum_required(VERSION 3.11)
project(sfizz-lv2)

find_package(PkgConfig REQUIRED)
pkg_check_modules(sfizz REQUIRED IMPORTED_TARGET sfizz)

# Force add relocation to the flags globally. It's ugly but what can you do...
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

# Export the compile_commands.json file
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(sfizz-lv2 SHARED sfizz.c)
set_target_properties(sfizz-lv2 PROPERTIES PREFIX "")
set_target_properties(sfizz-lv2 PROPERTIES OUTPUT_NAME "sfizz")
target_link_libraries(sfizz-lv2 sfizz)
if(UNIX)
target_compile_options(sfizz-lv2 PRIVATE -Wextra -pedantic -Wall -Werror)
endif()