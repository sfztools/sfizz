include (GNUInstallDirs)

set (SFIZZ_SOURCES
    sfizz/Synth.cpp
    sfizz/FilePool.cpp
    sfizz/FilterPool.cpp
    sfizz/EQPool.cpp
    sfizz/Region.cpp
    sfizz/Voice.cpp
    sfizz/ScopedFTZ.cpp
    sfizz/MidiState.cpp
    sfizz/SfzHelpers.cpp
    sfizz/Oversampler.cpp
    sfizz/FloatEnvelopes.cpp
    sfizz/Logger.cpp
    sfizz/SfzFilter.cpp
    sfizz/Effects.cpp
    sfizz/effects/Nothing.cpp
    sfizz/effects/Lofi.cpp
)
include (SfizzSIMDSourceFiles)

# Parser core library
add_library (sfizz_parser STATIC)
target_sources (sfizz_parser PRIVATE sfizz/Parser.cpp sfizz/Opcode.cpp sfizz/SfzHelpers.cpp)
target_include_directories (sfizz_parser PUBLIC sfizz)
target_include_directories (sfizz_parser PUBLIC external)
target_link_libraries (sfizz_parser PUBLIC absl::strings)

# Sfizz static library
add_library(sfizz_static STATIC)
target_sources(sfizz_static PRIVATE ${SFIZZ_SOURCES} sfizz/sfizz_wrapper.cpp sfizz/sfizz.cpp)
target_include_directories (sfizz_static PUBLIC .)
target_include_directories (sfizz_static PUBLIC external)
target_link_libraries (sfizz_static PUBLIC absl::strings absl::span)
target_link_libraries (sfizz_static PRIVATE sfizz_parser absl::flat_hash_map Threads::Threads sfizz-sndfile sfizz-pugixml)
set_target_properties (sfizz_static PROPERTIES OUTPUT_NAME sfizz PUBLIC_HEADER "sfizz.h;sfizz.hpp")
if (WIN32)
    target_compile_definitions (sfizz_static PRIVATE _USE_MATH_DEFINES)
endif()

if (NOT MSVC)
    install (TARGETS sfizz_static
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    configure_file (${PROJECT_SOURCE_DIR}/scripts/sfizz.pc.in sfizz.pc @ONLY)
endif()
if(WIN32)
    include(VSTConfig)
    configure_file (${PROJECT_SOURCE_DIR}/scripts/innosetup.iss.in ${PROJECT_BINARY_DIR}/innosetup.iss @ONLY)
endif()

configure_file (${PROJECT_SOURCE_DIR}/scripts/Doxyfile.in ${PROJECT_SOURCE_DIR}/Doxyfile @ONLY)

add_library (sfizz::parser ALIAS sfizz_parser)
add_library (sfizz::sfizz ALIAS sfizz_static)
if (LIBATOMIC_FOUND)
    target_link_libraries (sfizz_static PRIVATE atomic)
endif()

# Shared library and installation target
if (SFIZZ_SHARED)
    add_library (sfizz_shared SHARED)
    target_sources(sfizz_shared PRIVATE ${SFIZZ_SOURCES} sfizz/sfizz_wrapper.cpp sfizz/sfizz.cpp)
    target_include_directories (sfizz_shared PRIVATE .)
    target_include_directories (sfizz_shared PRIVATE external)
    target_link_libraries (sfizz_shared PRIVATE absl::strings absl::span sfizz_parser absl::flat_hash_map Threads::Threads sfizz-sndfile sfizz-pugixml)
    if (WIN32)
        target_compile_definitions (sfizz_shared PRIVATE _USE_MATH_DEFINES)
    endif()
    target_compile_definitions(sfizz_shared PRIVATE SFIZZ_EXPORT_SYMBOLS)
    set_target_properties (sfizz_shared PROPERTIES OUTPUT_NAME sfizz PUBLIC_HEADER "sfizz.h;sfizz.hpp")
    set_property (TARGET sfizz_shared PROPERTY SOVERSION ${PROJECT_VERSION_MAJOR})
    sfizz_enable_lto_if_needed(sfizz_shared)

    if (NOT MSVC)
        install (TARGETS sfizz_shared
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()

    if (UNIX AND NOT APPLE)
        if (LIBATOMIC_FOUND)
            target_link_libraries (sfizz_shared PRIVATE atomic)
        endif()
    endif()
endif()

if (NOT MSVC)
    install (FILES ${CMAKE_BINARY_DIR}/src/sfizz.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)
endif()
