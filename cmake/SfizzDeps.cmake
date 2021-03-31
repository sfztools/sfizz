# Find system threads
find_package(Threads REQUIRED)

# Find OpenMP
find_package(OpenMP)
if(OPENMP_FOUND)
    add_library(sfizz_openmp INTERFACE)
    add_library(sfizz::openmp ALIAS sfizz_openmp)

    # OpenMP flags are provided as a space-separated string, we need a list
    if(NOT CMAKE_VERSION VERSION_LESS 3.9)
        separate_arguments(SFIZZ_OpenMP_C_OPTIONS NATIVE_COMMAND "${OpenMP_C_FLAGS}")
        separate_arguments(SFIZZ_OpenMP_CXX_OPTIONS NATIVE_COMMAND "${OpenMP_CXX_FLAGS}")
    elseif(CMAKE_HOST_WIN32)
        separate_arguments(SFIZZ_OpenMP_C_OPTIONS WINDOWS_COMMAND "${OpenMP_C_FLAGS}")
        separate_arguments(SFIZZ_OpenMP_CXX_OPTIONS WINDOWS_COMMAND "${OpenMP_CXX_FLAGS}")
    else()
        separate_arguments(SFIZZ_OpenMP_C_OPTIONS UNIX_COMMAND "${OpenMP_C_FLAGS}")
        separate_arguments(SFIZZ_OpenMP_CXX_OPTIONS UNIX_COMMAND "${OpenMP_CXX_FLAGS}")
    endif()

    target_compile_options(sfizz_openmp INTERFACE
        $<$<COMPILE_LANGUAGE:C>:${SFIZZ_OpenMP_C_OPTIONS}>
        $<$<COMPILE_LANGUAGE:CXX>:${SFIZZ_OpenMP_CXX_OPTIONS}>)
endif()

# Find macOS system libraries
if(APPLE)
    find_library(APPLE_COREFOUNDATION_LIBRARY "CoreFoundation")
    find_library(APPLE_FOUNDATION_LIBRARY "Foundation")
    find_library(APPLE_COCOA_LIBRARY "Cocoa")
    find_library(APPLE_CARBON_LIBRARY "Carbon")
    find_library(APPLE_OPENGL_LIBRARY "OpenGL")
    find_library(APPLE_ACCELERATE_LIBRARY "Accelerate")
    find_library(APPLE_QUARTZCORE_LIBRARY "QuartzCore")
    find_library(APPLE_AUDIOTOOLBOX_LIBRARY "AudioToolbox")
    find_library(APPLE_AUDIOUNIT_LIBRARY "AudioUnit")
    find_library(APPLE_COREAUDIO_LIBRARY "CoreAudio")
    find_library(APPLE_COREMIDI_LIBRARY "CoreMIDI")
endif()

# Set up macOS library paths
if(APPLE)
    # See https://stackoverflow.com/a/54103956
    # and https://stackoverflow.com/a/21692023
    # Apparently this is not needed in Travis CI using addons
    # but it is in Appveyor instead
    list(APPEND CMAKE_PREFIX_PATH /usr/local)
endif()

# Add Abseil
if(SFIZZ_USE_SYSTEM_ABSEIL)
    find_package(absl REQUIRED)
else()
    function(sfizz_add_vendor_abseil)
        set(BUILD_SHARED_LIBS OFF) # only changed at local scope
        add_subdirectory("external/abseil-cpp" EXCLUDE_FROM_ALL)
    endfunction()
    sfizz_add_vendor_abseil()
endif()

# The jsl utility library for C++
add_library(sfizz_jsl INTERFACE)
add_library(sfizz::jsl ALIAS sfizz_jsl)
target_include_directories(sfizz_jsl INTERFACE "external/jsl/include")

# The cxxopts library
add_library(sfizz_cxxopts INTERFACE)
add_library(sfizz::cxxopts ALIAS sfizz_cxxopts)
target_include_directories(sfizz_cxxopts INTERFACE "external/cxxopts")

# The sndfile library
if(SFIZZ_USE_SNDFILE OR SFIZZ_DEMOS OR SFIZZ_DEVTOOLS OR SFIZZ_BENCHMARKS)
    add_library(sfizz_sndfile INTERFACE)
    add_library(sfizz::sndfile ALIAS sfizz_sndfile)
    if(SFIZZ_USE_VCPKG OR CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        find_package(SndFile CONFIG REQUIRED)
        find_path(SNDFILE_INCLUDE_DIR "sndfile.hh")
        target_include_directories(sfizz_sndfile INTERFACE "${SNDFILE_INCLUDE_DIR}")
        target_link_libraries(sfizz_sndfile INTERFACE SndFile::sndfile)
    else()
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(SNDFILE "sndfile" REQUIRED)
        target_include_directories(sfizz_sndfile INTERFACE ${SNDFILE_INCLUDE_DIRS})
        if(SFIZZ_STATIC_DEPENDENCIES)
            target_link_libraries(sfizz_sndfile INTERFACE ${SNDFILE_STATIC_LIBRARIES})
        else()
            target_link_libraries(sfizz_sndfile INTERFACE ${SNDFILE_LIBRARIES})
        endif()
        link_directories(${SNDFILE_LIBRARY_DIRS})
    endif()
endif()

# The st_audiofile library
if(SFIZZ_USE_SNDFILE)
    set(ST_AUDIO_FILE_USE_SNDFILE ON CACHE BOOL "" FORCE)
    set(ST_AUDIO_FILE_EXTERNAL_SNDFILE "sfizz::sndfile" CACHE STRING ""  FORCE)
else()
    set(ST_AUDIO_FILE_USE_SNDFILE OFF CACHE BOOL "" FORCE)
    set(ST_AUDIO_FILE_EXTERNAL_SNDFILE "" CACHE STRING ""  FORCE)
endif()
add_subdirectory("external/st_audiofile" EXCLUDE_FROM_ALL)

# The simde library
add_library(sfizz_simde INTERFACE)
add_library(sfizz::simde ALIAS sfizz_simde)
if(SFIZZ_USE_SYSTEM_SIMDE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SIMDE "simde" REQUIRED)
    target_include_directories(sfizz_simde INTERFACE "${SIMDE_INCLUDE_DIRS}")
    if(NOT SIMDE_VERSION OR SIMDE_VERSION VERSION_LESS_EQUAL "0.7.2")
        message(WARNING "The version of SIMDe on this system has known issues. \
It is recommended to either update if a newer version is available, or use the \
version bundled with this package. Refer to following issues: \
simd-everywhere/simde#704, simd-everywhere/simde#706")
    endif()
else()
    target_include_directories(sfizz_simde INTERFACE "external/simde")
endif()
if(TARGET sfizz::openmp)
    target_link_libraries(sfizz_simde INTERFACE sfizz::openmp)
endif()

# The pugixml library
add_library(sfizz_pugixml STATIC "src/external/pugixml/src/pugixml.cpp")
add_library(sfizz::pugixml ALIAS sfizz_pugixml)
target_include_directories(sfizz_pugixml PUBLIC "src/external/pugixml/src")

# The spline library
add_library(sfizz_spline STATIC "src/external/spline/spline/spline.cpp")
add_library(sfizz::spline ALIAS sfizz_spline)
target_include_directories(sfizz_spline PUBLIC "src/external/spline")

# The tunings library
add_library(sfizz_tunings STATIC "src/external/tunings/src/Tunings.cpp")
add_library(sfizz::tunings ALIAS sfizz_tunings)
target_include_directories(sfizz_tunings PUBLIC "src/external/tunings/include")

# The hiir library
add_library(sfizz_hiir INTERFACE)
add_library(sfizz::hiir ALIAS sfizz_hiir)
target_include_directories(sfizz_hiir INTERFACE "src/external/hiir")

# The hiir filter designer
add_library(sfizz_hiir_polyphase_iir2designer STATIC
    "src/external/hiir/hiir/PolyphaseIir2Designer.cpp")
add_library(sfizz::hiir_polyphase_iir2designer ALIAS sfizz_hiir_polyphase_iir2designer)
target_link_libraries(sfizz_hiir_polyphase_iir2designer PUBLIC sfizz::hiir)

# The kissfft library
add_library(sfizz_kissfft STATIC
    "src/external/kiss_fft/kiss_fft.c"
    "src/external/kiss_fft/tools/kiss_fftr.c")
add_library(sfizz::kissfft ALIAS sfizz_kissfft)
target_include_directories(sfizz_kissfft
    PUBLIC "src/external/kiss_fft"
    PUBLIC "src/external/kiss_fft/tools")

# The cephes library
add_library(sfizz_cephes STATIC
    "external/cephes/src/chbevl.c"
    "external/cephes/src/i0.c")
add_library(sfizz::cephes ALIAS sfizz_cephes)

# The cpuid library
add_library(sfizz_cpuid STATIC
    "src/external/cpuid/src/cpuid/cpuinfo.cpp"
    "src/external/cpuid/src/cpuid/version.cpp")
add_library(sfizz::cpuid ALIAS sfizz_cpuid)
set_property(TARGET sfizz_cpuid PROPERTY CXX_STANDARD 11)
target_include_directories(sfizz_cpuid
    PUBLIC "src/external/cpuid/src"
    PRIVATE "src/external/cpuid/platform/src")

# The threadpool library
add_library(sfizz_threadpool INTERFACE)
add_library(sfizz::threadpool ALIAS sfizz_threadpool)
target_include_directories(sfizz_threadpool INTERFACE "external/threadpool")

# The atomic_queue library
add_library(sfizz_atomic_queue INTERFACE)
add_library(sfizz::atomic_queue ALIAS sfizz_atomic_queue)
target_include_directories(sfizz_atomic_queue INTERFACE "external/atomic_queue/include")

# The ghc::filesystem library
add_library(sfizz_filesystem INTERFACE)
add_library(sfizz::filesystem ALIAS sfizz_filesystem)
target_include_directories(sfizz_filesystem INTERFACE "external/filesystem/include")

# The atomic library
add_library(sfizz_atomic INTERFACE)
add_library(sfizz::atomic ALIAS sfizz_atomic)
if(UNIX AND NOT APPLE)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/check_libatomic")
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/check_libatomic/check_libatomic.c" "int main() { return 0; }")
    try_compile(SFIZZ_LINK_LIBATOMIC "${CMAKE_CURRENT_BINARY_DIR}/check_libatomic"
        SOURCES "${CMAKE_CURRENT_BINARY_DIR}/check_libatomic/check_libatomic.c"
        LINK_LIBRARIES "atomic")
    if(SFIZZ_LINK_LIBATOMIC)
        target_link_libraries(sfizz_atomic INTERFACE "atomic")
    endif()
endif()

# The jack library
if(SFIZZ_JACK)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JACK "jack" REQUIRED)
elseif()
    find_package(PkgConfig)
    if(PKGCONFIG_FOUND)
        pkg_check_modules(JACK "jack")
    endif()
endif()
if(JACK_FOUND)
    add_library(sfizz_jacklib INTERFACE)
    add_library(sfizz::jack ALIAS sfizz_jacklib)
    target_include_directories(sfizz_jacklib INTERFACE ${JACK_INCLUDE_DIRS})
    target_link_libraries(sfizz_jacklib INTERFACE ${JACK_LIBRARIES})
    link_directories(${JACK_LIBRARY_DIRS})
endif()

# The Qt library
find_package(Qt5 COMPONENTS Widgets)

# The fmidi library
add_library(sfizz_fmidi STATIC
    "external/fmidi/sources/fmidi/fmidi.h"
    "external/fmidi/sources/fmidi/fmidi_mini.cpp")
add_library(sfizz::fmidi ALIAS sfizz_fmidi)
target_include_directories(sfizz_fmidi PUBLIC "external/fmidi/sources")
target_compile_definitions(sfizz_fmidi PUBLIC "FMIDI_STATIC=1" "FMIDI_DISABLE_DESCRIBE_API=1")

# The samplerate library
find_package(PkgConfig)
if(PKGCONFIG_FOUND)
    pkg_check_modules(SAMPLERATE "samplerate")
    if(SAMPLERATE_FOUND)
        add_library(sfizz_samplerate INTERFACE)
        add_library(sfizz::samplerate ALIAS sfizz_samplerate)
        target_include_directories(sfizz_samplerate INTERFACE ${SAMPLERATE_INCLUDE_DIRS})
        target_link_libraries(sfizz_samplerate INTERFACE ${SAMPLERATE_LIBRARIES})
        link_directories(${SAMPLERATE_LIBRARY_DIRS})
    endif()
endif()
if(NOT TARGET sfizz::samplerate)
    find_library(SAMPLERATE_LIBRARY "samplerate")
    find_path(SAMPLERATE_INCLUDE_DIR "samplerate.h")
    message(STATUS "Checking samplerate library: ${SAMPLERATE_LIBRARY}")
    message(STATUS "Checking samplerate includes: ${SAMPLERATE_INCLUDE_DIR}")
    if(SAMPLERATE_LIBRARY AND SAMPLERATE_INCLUDE_DIR)
        add_library(sfizz_samplerate INTERFACE)
        add_library(sfizz::samplerate ALIAS sfizz_samplerate)
        target_include_directories(sfizz_samplerate INTERFACE "${SAMPLERATE_INCLUDE_DIR}")
        target_link_libraries(sfizz_samplerate INTERFACE "${SAMPLERATE_LIBRARY}")
    endif()
endif()

# The benchmark library
if(SFIZZ_BENCHMARKS)
    find_package(benchmark CONFIG REQUIRED)
endif()
