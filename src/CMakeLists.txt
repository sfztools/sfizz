set (SFIZZ_SOURCES
    sfizz/Synth.cpp
    sfizz/FilePool.cpp
    sfizz/Region.cpp
    sfizz/Voice.cpp
    sfizz/ScopedFTZ.cpp
    sfizz/MidiState.cpp
    sfizz/SfzHelpers.cpp
    sfizz/Oversampler.cpp
    sfizz/FloatEnvelopes.cpp
    sfizz/Logger.cpp
)
include (SfizzSIMDSourceFilesCheck)

# Parser core library
add_library (sfizz_parser STATIC)
target_sources (sfizz_parser PRIVATE sfizz/Parser.cpp sfizz/Opcode.cpp sfizz/SfzHelpers.cpp)
target_include_directories (sfizz_parser PUBLIC sfizz)
target_include_directories (sfizz_parser PUBLIC external)
target_link_libraries (sfizz_parser PRIVATE absl::strings)

# Sfizz core library
add_library(sfizz STATIC)
target_sources(sfizz PRIVATE ${SFIZZ_SOURCES} sfizz/sfizz_wrapper.cpp)
target_include_directories (sfizz PUBLIC .)
target_include_directories (sfizz PUBLIC external)
target_link_libraries (sfizz PUBLIC absl::strings absl::span)
target_link_libraries (sfizz PRIVATE sfizz_parser absl::flat_hash_map Threads::Threads)
sfizz_link_libsndfile(sfizz)
sfizz_enable_lto_if_needed(sfizz)

if (UNIX AND NOT APPLE)
    target_link_libraries (sfizz PRIVATE atomic)
endif()

add_library (sfizz::parser ALIAS sfizz_parser)
add_library (sfizz::sfizz ALIAS sfizz)

# install(TARGETS sfizz_parser DESTINATION . EXCLUDE_FROM_ALL)
# install(TARGETS sfizz DESTINATION . EXCLUDE_FROM_ALL)

# Shared library and installation target
if (SFIZZ_SHARED)
    add_library (sfizz_shared SHARED)
    target_compile_options(sfizz_shared PRIVATE -DSFIZZ_EXPORT_SYMBOLS)
    target_sources(sfizz_shared PRIVATE ${SFIZZ_SOURCES} sfizz/sfizz_wrapper.cpp)
    target_include_directories (sfizz_shared PRIVATE .)
    target_link_libraries (sfizz_shared PRIVATE sfizz_parser absl::flat_hash_map Threads::Threads absl::strings absl::span)
    set_target_properties (sfizz_shared PROPERTIES OUTPUT_NAME sfizz PUBLIC_HEADER sfizz.h)
    sfizz_link_libsndfile(sfizz_shared)
    sfizz_enable_lto_if_needed(sfizz_shared)
    configure_file (${PROJECT_SOURCE_DIR}/scripts/sfizz.pc.in sfizz.pc @ONLY)
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
