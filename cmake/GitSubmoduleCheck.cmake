find_package(Git REQUIRED)

# https://gist.github.com/scivision/bb1d47a9529e153617414e91ff5390af

function(git_submodule_check dir)
    if(NOT EXISTS "${dir}/CMakeLists.txt")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- ${dir}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            COMMAND_ERROR_IS_FATAL ANY)
    endif()
#   Add a Git submodule directory to CMake, assuming the Git submodule directory is a CMake project.
#   add_subdirectory(${dir} EXCLUDE_FROM_ALL)
endfunction()
