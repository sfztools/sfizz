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

add_library (sfizz_objects OBJECT)
target_sources(sfizz_objects PRIVATE ${SFIZZ_SOURCES})
target_include_directories (sfizz_objects PUBLIC external)
target_link_libraries (sfizz_objects PUBLIC absl::strings absl::span)
target_link_libraries (sfizz_objects PRIVATE sfizz_parser absl::flat_hash_map Threads::Threads)
if (UNIX AND NOT APPLE)
    target_link_libraries (sfizz_objects PRIVATE atomic)
endif()
sfizz_link_libsndfile(sfizz_objects)

# Sfizz core library
add_library(sfizz_static STATIC)
target_sources(sfizz_static PRIVATE sfizz/sfizz_wrapper.cpp)
target_link_libraries (sfizz_static PRIVATE sfizz_objects)
target_include_directories (sfizz_static PUBLIC .)
target_include_directories (sfizz_static PUBLIC external) # Object dependency is not transitive?
target_link_libraries (sfizz_static PUBLIC absl::strings absl::span) # Object dependency is not transitive?
sfizz_enable_lto_if_needed(sfizz_static)

add_library (sfizz::parser ALIAS sfizz_parser)
add_library (sfizz::sfizz ALIAS sfizz_static)

# install(TARGETS sfizz_parser DESTINATION . EXCLUDE_FROM_ALL)
# install(TARGETS sfizz DESTINATION . EXCLUDE_FROM_ALL)

# Shared library and installation target
if (SFIZZ_SHARED)
    add_library (sfizz_shared SHARED)
    target_sources(sfizz_shared PRIVATE sfizz/sfizz_wrapper.cpp)
    target_include_directories (sfizz_shared PRIVATE .)
    target_link_libraries (sfizz_shared PRIVATE sfizz_objects)
    target_compile_definitions(sfizz_shared PRIVATE SFIZZ_EXPORT_SYMBOLS)
    set_target_properties (sfizz_shared PROPERTIES OUTPUT_NAME sfizz PUBLIC_HEADER sfizz.h)
    # sfizz_link_libsndfile(sfizz_shared)
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
