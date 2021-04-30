# Puredata plugin specific settings
include(PuredataConfig)

set(PUREDATA_BINARY_DIR "${PROJECT_BINARY_DIR}/pd/sfizz")

set(PUREDATA_RESOURCES
    "sfizz~-help.pd"
    "example.sfz")

function(copy_puredata_resources TARGET SOURCE_DIR DESTINATION_DIR)
    set(_deps)
    foreach(res ${PUREDATA_RESOURCES})
        get_filename_component(_dir "${res}" DIRECTORY)
        file(MAKE_DIRECTORY "${DESTINATION_DIR}/${_dir}")
        add_custom_command(
            OUTPUT "${DESTINATION_DIR}/${res}"
            COMMAND "${CMAKE_COMMAND}" "-E" "copy"
                    "${SOURCE_DIR}/${res}" "${DESTINATION_DIR}/${res}"
            DEPENDS "${SOURCE_DIR}/${res}")
        list(APPEND _deps "${DESTINATION_DIR}/${res}")
    endforeach()
    add_custom_target("${TARGET}_puredata_resources" DEPENDS ${_deps})
    add_dependencies("${TARGET}" "${TARGET}_puredata_resources")
endfunction()

add_library(sfizz_puredata MODULE sfizz_puredata.c)
target_include_directories(sfizz_puredata PRIVATE "${PUREDATA_INCLUDE_DIR}")
target_link_libraries(sfizz_puredata PRIVATE sfizz::sfizz)

set_target_properties(sfizz_puredata PROPERTIES
    PREFIX ""
    SUFFIX "${PUREDATA_SUFFIX}"
    OUTPUT_NAME "sfizz"
    LIBRARY_OUTPUT_DIRECTORY "${PUREDATA_BINARY_DIR}/$<0:>")

copy_puredata_resources(sfizz_puredata
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PUREDATA_BINARY_DIR}")
