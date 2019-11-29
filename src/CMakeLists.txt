project(sfizz)

set (SFIZZ_SOURCES
    sfizz/Synth.cpp
    sfizz/FilePool.cpp
    sfizz/Region.cpp
    sfizz/Voice.cpp
    sfizz/ScopedFTZ.cpp
    sfizz/SfzHelpers.cpp
    sfizz/FloatEnvelopes.cpp
)
include (SfizzSIMDSourceFilesCheck)

# Parser core library
add_library (sfizz_parser STATIC)
target_sources (sfizz_parser PRIVATE sfizz/Parser.cpp sfizz/Opcode.cpp)
target_include_directories (sfizz_parser PUBLIC sfizz)
target_include_directories (sfizz_parser PUBLIC external)
target_link_libraries (sfizz_parser PRIVATE absl::strings)

# Sfizz core library
add_library (sfizz STATIC ${SFIZZ_SOURCES} sfizz/sfizz_wrapper.cpp)
target_include_directories (sfizz PUBLIC sfizz)
target_include_directories (sfizz PUBLIC .)
target_include_directories (sfizz PUBLIC external)
target_link_libraries (sfizz PRIVATE sfizz_parser)
find_package (Threads REQUIRED)
target_link_libraries (sfizz PUBLIC absl::strings)
target_link_libraries (sfizz PRIVATE sndfile absl::flat_hash_map Threads::Threads)

if (UNIX)
    target_link_libraries (sfizz PUBLIC atomic)
endif()

sfizz_enable_lto_if_needed(sfizz_parser)
sfizz_enable_lto_if_needed(sfizz)

add_library (sfizz::parser ALIAS sfizz_parser)
add_library (sfizz::sfizz ALIAS sfizz)

# install(TARGETS sfizz_parser DESTINATION . EXCLUDE_FROM_ALL)
# install(TARGETS sfizz DESTINATION . EXCLUDE_FROM_ALL)

# Shared library and installation target
if (SFIZZ_SHARED)
    add_library (sfizz_shared SHARED sfizz.h)
    set_target_properties (sfizz_shared PROPERTIES OUTPUT_NAME sfizz PUBLIC_HEADER sfizz.h)
    target_link_libraries (sfizz_shared sfizz::sfizz)
    configure_file (${CMAKE_SOURCE_DIR}/scripts/sfizz.pc.in sfizz.pc @ONLY)
    if (UNIX)
        include (GNUInstallDirs)
        install (TARGETS sfizz_shared
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        install (FILES
            ${CMAKE_BINARY_DIR}/src/sfizz.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
    endif()
endif()
