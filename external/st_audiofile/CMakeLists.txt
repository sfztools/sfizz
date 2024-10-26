cmake_minimum_required(VERSION 3.5)
project(st_audiofile)

option(ST_AUDIO_FILE_USE_SNDFILE "Use sndfile" OFF)
set(ST_AUDIO_FILE_EXTERNAL_SNDFILE "" CACHE STRING "Name of external sndfile target")

add_library(st_audiofile STATIC EXCLUDE_FROM_ALL
    "src/st_audiofile.c"
    "src/st_audiofile_common.c"
    "src/st_audiofile_sndfile.c")
target_include_directories(st_audiofile
    PUBLIC "src")

###
add_library(st_audiofile_formats STATIC EXCLUDE_FROM_ALL
    "src/st_audiofile_libs.c")
target_include_directories(st_audiofile_formats
    PUBLIC "src"
    PUBLIC "thirdparty/dr_libs"
    PUBLIC "thirdparty/stb_vorbis")

add_subdirectory("thirdparty/libaiff" EXCLUDE_FROM_ALL)
target_link_libraries(st_audiofile_formats PUBLIC aiff::aiff)

set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(WAVPACK_BUILD_COOLEDIT_PLUGIN OFF)
set(WAVPACK_BUILD_PROGRAMS OFF)
set(WAVPACK_BUILD_WINAMP_PLUGIN OFF)
set(WAVPACK_ENABLE_LIBCRYPTO OFF)
set(WAVPACK_INSTALL_CMAKE_MODULE OFF)
set(WAVPACK_INSTALL_DOCS OFF)
set(WAVPACK_INSTALL_PKGCONFIG_MODULE OFF)
set(WAVPACK_ENABLE_ASM OFF)
set(WAVPACK_BUILD_DOCS OFF)
set(WAVPACK_LEGACY ON)

add_subdirectory("thirdparty/wavpack" EXCLUDE_FROM_ALL)
target_link_libraries(st_audiofile_formats PUBLIC wavpack)

###
if(NOT ST_AUDIO_FILE_USE_SNDFILE)
    target_link_libraries(st_audiofile PRIVATE st_audiofile_formats)
else()
    target_compile_definitions(st_audiofile
        PUBLIC "ST_AUDIO_FILE_USE_SNDFILE=1")
    if(ST_AUDIO_FILE_EXTERNAL_SNDFILE)
        target_link_libraries(st_audiofile
            PUBLIC "${ST_AUDIO_FILE_EXTERNAL_SNDFILE}")
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(Sndfile "sndfile" REQUIRED)
        target_include_directories(st_audiofile PUBLIC ${Sndfile_INCLUDE_DIRS})
        target_link_libraries(st_audiofile PUBLIC ${Sndfile_LIBRARIES})
        link_directories(${Sndfile_LIBRARY_DIRS})
    endif()
endif()

###
add_executable(st_info "src/st_info.c")
target_link_libraries(st_info PRIVATE st_audiofile)
