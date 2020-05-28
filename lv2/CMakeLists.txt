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
if (SFIZZ_LV2_UI)
    list(APPEND LV2PLUGIN_TTL_SRC_FILES
        ${PROJECT_NAME}_ui.ttl.in)
endif()
source_group("Turtle Files" FILES
    ${LV2PLUGIN_TTL_SRC_FILES}
)
add_library (${LV2PLUGIN_PRJ_NAME} MODULE
    ${PROJECT_NAME}.c
    atomic_compat.h
    ${LV2PLUGIN_TTL_SRC_FILES})
add_library (${LV2PLUGIN_PRJ_NAME}_ui MODULE
    ${PROJECT_NAME}_ui.cpp)
target_link_libraries (${LV2PLUGIN_PRJ_NAME} ${PROJECT_NAME}::${PROJECT_NAME})

if (SFIZZ_LV2_UI)
    add_library (${LV2PLUGIN_PRJ_NAME}_ui MODULE ${PROJECT_NAME}_ui.cpp)
    target_link_libraries (${LV2PLUGIN_PRJ_NAME}_ui sfizz_editor)
endif()

# Explicitely strip all symbols on Linux but lv2_descriptor()
# MacOS linker does not support this apparently https://bugs.webkit.org/show_bug.cgi?id=144555
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    file(COPY lv2.version DESTINATION ${CMAKE_BINARY_DIR}/lv2)
    target_link_libraries(${LV2PLUGIN_PRJ_NAME} "-Wl,--version-script=lv2.version")
    # target_link_libraries(${LV2PLUGIN_PRJ_NAME} "-Wl,-u,lv2_descriptor")
    if (SFIZZ_LV2_UI)
        file(COPY lv2ui.version DESTINATION ${CMAKE_BINARY_DIR}/lv2)
        target_link_libraries(${LV2PLUGIN_PRJ_NAME}_ui "-Wl,--version-script=lv2ui.version")
        # target_link_libraries(${LV2PLUGIN_PRJ_NAME}_ui "-Wl,-u,lv2ui_descriptor")
    endif()
endif()
target_include_directories(${LV2PLUGIN_PRJ_NAME} PRIVATE . external/ardour)
sfizz_enable_lto_if_needed (${LV2PLUGIN_PRJ_NAME})
if (MINGW)
    set_target_properties (${LV2PLUGIN_PRJ_NAME} PROPERTIES LINK_FLAGS "-static")
endif()
if (SFIZZ_LV2_UI)
    target_include_directories(${LV2PLUGIN_PRJ_NAME}_ui PRIVATE . external/ardour)
    sfizz_enable_lto_if_needed (${LV2PLUGIN_PRJ_NAME}_ui)
    if (MINGW)
        set_target_properties (${LV2PLUGIN_PRJ_NAME}_ui PROPERTIES LINK_FLAGS "-static")
    endif()
endif()

# Remove the "lib" prefix, rename the target name and build it in the .lv build dir
# <build_dir>/lv2/<plugin_name>_lv2.<ext> to
# <build_dir>/lv2/<plugin_name>.lv2/<plugin_name>.<ext>
set_target_properties (${LV2PLUGIN_PRJ_NAME} PROPERTIES PREFIX "")
set_target_properties (${LV2PLUGIN_PRJ_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
set_target_properties (${LV2PLUGIN_PRJ_NAME} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")

if (SFIZZ_LV2_UI)
    set_target_properties (${LV2PLUGIN_PRJ_NAME}_ui PROPERTIES PREFIX "")
    set_target_properties (${LV2PLUGIN_PRJ_NAME}_ui PROPERTIES OUTPUT_NAME "${PROJECT_NAME}_ui")
    set_target_properties (${LV2PLUGIN_PRJ_NAME}_ui PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}")
endif()

# Generate *.ttl files from *.in sources,
# create the destination directory if it doesn't exists and copy needed files
file (MAKE_DIRECTORY ${PROJECT_BINARY_DIR})
configure_file (manifest.ttl.in ${PROJECT_BINARY_DIR}/manifest.ttl)
configure_file (${PROJECT_NAME}.ttl.in ${PROJECT_BINARY_DIR}/${PROJECT_NAME}.ttl)
if (SFIZZ_LV2_UI)
    configure_file (${PROJECT_NAME}_ui.ttl.in ${PROJECT_BINARY_DIR}/${PROJECT_NAME}_ui.ttl)
endif()
configure_file (LICENSE.md.in ${PROJECT_BINARY_DIR}/LICENSE.md)
if (SFIZZ_USE_VCPKG OR SFIZZ_STATIC_DEPENDENCIES OR CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    file(COPY "lgpl-3.0.txt" DESTINATION ${PROJECT_BINARY_DIR})
endif()

# Copy resource files into the bundle
set(LV2_RESOURCES
    DefaultInstrument.sfz
    DefaultScale.scl)
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${PROJECT_BINARY_DIR}/Resources")
foreach(res ${LV2_RESOURCES})
    file (COPY "${CMAKE_CURRENT_SOURCE_DIR}/resources/${res}"
        DESTINATION "${PROJECT_BINARY_DIR}/Resources")
endforeach()

# Installation
if (NOT MSVC)
    install (DIRECTORY ${PROJECT_BINARY_DIR} DESTINATION ${LV2PLUGIN_INSTALL_DIR}
        COMPONENT "lv2")
endif()
