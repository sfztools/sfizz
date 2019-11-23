project(sfizz)

set(SFIZZ_SOURCES
    sfizz/Synth.cpp
    sfizz/FilePool.cpp
    sfizz/Region.cpp
    sfizz/Voice.cpp
    sfizz/ScopedFTZ.cpp
    sfizz/SfzHelpers.cpp
    sfizz/FloatEnvelopes.cpp
)

# Check SIMD
include(CheckIncludeFiles)
CHECK_INCLUDE_FILES(x86intrin.h HAVE_X86INTRIN_H)
CHECK_INCLUDE_FILES(intrin.h HAVE_INTRIN_H)
if (!APPLE)
    CHECK_INCLUDE_FILES(arm_neon.h HAVE_ARM_NEON_H)
endif()

# SIMD checks
if (HAVE_X86INTRIN_H AND UNIX)
    add_compile_options(-DHAVE_X86INTRIN_H)
    set(SFIZZ_SIMD_SOURCES sfizz/SIMDSSE.cpp)
elseif (HAVE_INTRIN_H AND WIN32)
    add_compile_options(/DHAVE_INTRIN_H)
    set(SFIZZ_SIMD_SOURCES sfizz/SIMDSSE.cpp)
elseif (HAVE_ARM_NEON_H AND UNIX)
    add_compile_options(-DHAVE_ARM_NEON_H)
    add_compile_options(-mfpu=neon-fp-armv8)
    add_compile_options(-march=native)
    add_compile_options(-mtune=cortex-a53)
    add_compile_options(-funsafe-math-optimizations)
    set(SFIZZ_SIMD_SOURCES sfizz/SIMDDummy.cpp)
else()
    set(SFIZZ_SIMD_SOURCES sfizz/SIMDDummy.cpp)
endif()

set(SFIZZ_SOURCES ${SFIZZ_SOURCES} ${SFIZZ_SIMD_SOURCES})

add_library(sfizz_parser STATIC)
target_sources(sfizz_parser PRIVATE sfizz/Parser.cpp sfizz/Opcode.cpp)
target_include_directories(sfizz_parser PUBLIC sfizz)
target_include_directories(sfizz_parser PUBLIC external)
target_link_libraries(sfizz_parser PRIVATE absl::strings)

add_library(sfizz STATIC ${SFIZZ_SOURCES} sfizz/sfizz_wrapper.cpp)
target_include_directories(sfizz PUBLIC sfizz)
target_include_directories(sfizz PUBLIC external)
target_link_libraries(sfizz PRIVATE sfizz_parser)
find_package(Threads REQUIRED)
target_link_libraries(sfizz PUBLIC absl::strings)
target_link_libraries(sfizz PRIVATE sndfile absl::flat_hash_map Threads::Threads)
if(UNIX)
    target_link_libraries(sfizz PUBLIC atomic)
endif(UNIX)

add_library(sfizz::parser ALIAS sfizz_parser)
add_library(sfizz::sfizz ALIAS sfizz)

# install(TARGETS sfizz_parser DESTINATION . EXCLUDE_FROM_ALL)
# install(TARGETS sfizz DESTINATION . EXCLUDE_FROM_ALL)

# Shared library and installation target

if (SFIZZ_SHARED)
    add_library(sfizz_shared SHARED sfizz.h)
    set_target_properties(sfizz_shared PROPERTIES OUTPUT_NAME sfizz PUBLIC_HEADER sfizz.h)
    target_link_libraries(sfizz_shared sfizz::sfizz)
    configure_file(${CMAKE_SOURCE_DIR}/scripts/sfizz.pc.in sfizz.pc @ONLY)
    if(UNIX)
        include(GNUInstallDirs)
        install(TARGETS sfizz_shared 
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
        install(FILES 
            ${CMAKE_BINARY_DIR}/src/sfizz.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
    endif()
endif(SFIZZ_SHARED)