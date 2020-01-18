set (LV2PLUGIN_PRJ_NAME "${PROJECT_NAME}_lv2")

# Set the build directory as <build_dir>/lv2/<plugin_name>.lv2/
set (PROJECT_BINARY_DIR "${PROJECT_BINARY_DIR}/${PROJECT_NAME}.lv2")

# LV2 plugin specific settings
include (LV2Config)

# Keep non build turtle files in IDE
set (LV2PLUGIN_TTL_SRC_FILES
    manifest.ttl.in
    ${PROJECT_NAME}.ttl.in
)
add_library (${LV2PLUGIN_PRJ_NAME} SHARED ${PROJECT_NAME}.c ${LV2PLUGIN_TTL_SRC_FILES})
target_link_libraries (${LV2PLUGIN_PRJ_NAME} ${PROJECT_NAME}::${PROJECT_NAME})
target_include_directories(${LV2PLUGIN_PRJ_NAME} PRIVATE .)
sfizz_enable_lto_if_needed (${LV2PLUGIN_PRJ_NAME})

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

# Installation
install (DIRECTORY ${PROJECT_BINARY_DIR} DESTINATION ${LV2PLUGIN_INSTALL_DIR})
