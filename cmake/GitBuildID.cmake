# Generate the Git Build ID information into a C source file
# This does not overwrite the file if the contents are up-to-date.
#
# Arguments:
#   SOURCE_DIR   The root directory of the project, expected to be a Git repo
#   OUTPUT_FILE  The file which gets written
#   PREFIX       An optional prefix for the constant name

find_package(Git QUIET)

function(make_git_build_id)
    set(oneValueArgs SOURCE_DIR OUTPUT_FILE PREFIX)
    cmake_parse_arguments(GBI "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    get_filename_component(OUTPUT_NAME "${GBI_OUTPUT_FILE}" NAME)
    get_filename_component(OUTPUT_DIR  "${GBI_OUTPUT_FILE}" DIRECTORY)
    message("(Git Build ID) Generating ${OUTPUT_NAME}")

    file(MAKE_DIRECTORY "${OUTPUT_DIR}")

    if(GIT_FOUND)
        execute_process(COMMAND "${GIT_EXECUTABLE}" "rev-parse" "--short" "HEAD"
            WORKING_DIRECTORY "${GBI_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_COMMIT_ID
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        set(GIT_COMMIT_ID "")
        message("(Git Build ID) Error: could not find Git")
    endif()

    file(WRITE "${GBI_OUTPUT_FILE}.temp" "const char* ${GBI_PREFIX}GitBuildId = \"${GIT_COMMIT_ID}\";\n")
    execute_process(COMMAND "${CMAKE_COMMAND}" "-E" "copy_if_different" "${GBI_OUTPUT_FILE}.temp" "${GBI_OUTPUT_FILE}")
    file(REMOVE "${GBI_OUTPUT_FILE}.temp")
endfunction()
