set (LV2PLUGIN_PRJ_NAME "${PROJECT_NAME}_lv2")

# Set the build directory as <build_dir>/lv2/<plugin_name>.lv2/
set (PROJECT_BINARY_DIR "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.lv2")

# C99 or higher is needed
set(CMAKE_C_STANDARD 99)

# LV2 plugin specific settings
include (LV2Config)

# Keep non build turtle files in IDE
set (LV2PLUGIN_TTL_SRC_FILES
    manifest.ttl.in
    ${PROJECT_NAME}.ttl.in
)
add_library (${LV2PLUGIN_PRJ_NAME} MODULE ${PROJECT_NAME}.c ${LV2PLUGIN_TTL_SRC_FILES})
target_link_libraries (${LV2PLUGIN_PRJ_NAME} ${PROJECT_NAME}::${PROJECT_NAME})
# Explicitely strip all symbols on Linux but lv2_descriptor()
# MacOS linker does not support this apparently https://bugs.webkit.org/show_bug.cgi?id=144555
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    file(COPY lv2.version DESTINATION ${CMAKE_BINARY_DIR}/lv2)
    target_link_libraries(${LV2PLUGIN_PRJ_NAME} "-Wl,--version-script=lv2.version")
    # target_link_libraries(${LV2PLUGIN_PRJ_NAME} "-Wl,-u,lv2_descriptor")
endif()
target_include_directories(${LV2PLUGIN_PRJ_NAME} PRIVATE . external/ardour)
sfizz_enable_lto_if_needed (${LV2PLUGIN_PRJ_NAME})
if (MINGW)
    set_target_properties (${LV2PLUGIN_PRJ_NAME} PROPERTIES LINK_FLAGS "-static")
endif()

# Remove the "lib" prefix, rename the target name and build it in the .lv build dir
# <build_dir>/lv2/<plugin_name>_lv2.<ext> to
# <build_dir>/lv2/<plugin_name>.lv2/<plugin_name>.<ext>
set_target_properties (${LV2PLUGIN_PRJ_NAME} PROPERTIES PREFIX "")
set_target_properties (${LV2PLUGIN_PRJ_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
set_target_properties (${LV2PLUGIN_PRJ_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

# Generate *.ttl files from *.in sources,
# create the destination directory if it doesn't exists and copy needed files
file (MAKE_DIRECTORY ${PROJECT_BINARY_DIR})
configure_file (manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
configure_file (${PROJECT_NAME}.ttl.in ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.ttl)
configure_file (LICENSE.md.in ${PROJECT_BINARY_DIR}/LICENSE.md)
if (SFIZZ_USE_VCPKG OR SFIZZ_STATIC_LIBSNDFILE OR CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    file(COPY "lgpl-3.0.txt" DESTINATION ${PROJECT_BINARY_DIR})
endif()

# Installation
if (NOT MSVC)
    install (DIRECTORY ${PROJECT_BINARY_DIR} DESTINATION ${LV2PLUGIN_INSTALL_DIR})
endif()
