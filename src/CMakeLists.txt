project(sfizz)

set(SFIZZ_SOURCES
    Synth.cpp
    FilePool.cpp
    Region.cpp
    Voice.cpp
    ScopedFTZ.cpp
    SfzHelpers.cpp
    FloatEnvelopes.cpp
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
    set(SFIZZ_SIMD_SOURCES SIMDSSE.cpp)
elseif (HAVE_INTRIN_H AND WIN32)
    add_compile_options(/DHAVE_INTRIN_H)
    set(SFIZZ_SIMD_SOURCES SIMDSSE.cpp)
elseif (HAVE_ARM_NEON_H AND UNIX)
    add_compile_options(-DHAVE_ARM_NEON_H)
    add_compile_options(-mfpu=neon-fp-armv8)
    add_compile_options(-march=native)
    add_compile_options(-mtune=cortex-a53)
    add_compile_options(-funsafe-math-optimizations)
    set(SFIZZ_SIMD_SOURCES SIMDNEON.cpp)
else()
    set(SFIZZ_SIMD_SOURCES SIMDDummy.cpp)
endif()

set(SFIZZ_SOURCES ${SFIZZ_SOURCES} ${SFIZZ_SIMD_SOURCES})

# check_include_file_cxx("filesystem" HAVE_STD_FILESYSTEM)
# if (HAVE_STD_FILESYSTEM)
#     if (CMAKE_CXX_COMPILER_ID MATCHES "Clang") 
#         # add_compile_options(-DUSE_STD_FILESYSTEM)
#         if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
#             link_libraries(c++fs)
#         endif()
#     elseif (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
#         link_libraries(stdc++fs)
#     elseif (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#         add_compile_options(/DINCLUDE_STD_FILESYSTEM)
#         add_compile_options(/DUSE_STD_FILESYSTEM)
#     endif()
# endif()


add_library(sfizz_parser STATIC)
target_sources(sfizz_parser PRIVATE Parser.cpp Opcode.cpp)
target_include_directories(sfizz_parser PUBLIC .)
target_link_libraries(sfizz_parser PRIVATE absl::strings)

add_library(sfizz STATIC ${SFIZZ_SOURCES})
target_link_libraries(sfizz PRIVATE sfizz_parser)
target_include_directories(sfizz PUBLIC .)
find_package(Threads REQUIRED)
target_link_libraries(sfizz PRIVATE Threads::Threads)
if(UNIX)
target_link_libraries(sfizz PUBLIC atomic)
endif(UNIX)

target_link_libraries(sfizz PUBLIC absl::strings)
target_link_libraries(sfizz PRIVATE sndfile absl::flat_hash_map)

install(TARGETS sfizz_parser DESTINATION . EXCLUDE_FROM_ALL)
install(TARGETS sfizz DESTINATION . EXCLUDE_FROM_ALL)

add_library(sfizz::parser ALIAS sfizz_parser)
add_library(sfizz::sfizz ALIAS sfizz)

add_library(sfizz_shared SHARED)
set_target_properties(sfizz_shared PROPERTIES
    OUTPUT_NAME sfizz
    PUBLIC_HEADER sfizz.h)
# set_target_properties(sfizz_shared PROPERTIES POSITION_INDEPENDENT_CODE ON)
target_sources(sfizz_shared PRIVATE sfizz.cpp)
target_link_libraries(sfizz_shared sfizz::sfizz)
configure_file(sfizz.pc.in sfizz.pc @ONLY)

if(UNIX)
    include(GNUInstallDirs)
    install(TARGETS sfizz_shared 
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/static
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    install(FILES 
        ${CMAKE_BINARY_DIR}/sfizz/sfizz.pc DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
endif()