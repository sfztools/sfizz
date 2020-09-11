include (GNUInstallDirs)

add_subdirectory(external/kiss_fft)
add_subdirectory(external/cpuid)

set (FAUST_FILES
    sfizz/dsp/filters/filters_modulable.dsp
    sfizz/dsp/filters/rbj_filters.dsp
    sfizz/dsp/filters/sallenkey_modulable.dsp
    sfizz/dsp/filters/sfz_filters.dsp
    sfizz/effects/dsp/limiter.dsp
    sfizz/effects/dsp/resonant_string.dsp
    sfizz/effects/dsp/compressor.dsp
    sfizz/effects/dsp/gate.dsp
    sfizz/effects/dsp/disto_stage.dsp
    sfizz/effects/dsp/fverb.dsp)
source_group ("Faust Files" FILES ${FAUST_FILES})

set (SFIZZ_HEADERS
    sfizz/ADSREnvelope.h
    sfizz/AudioBuffer.h
    sfizz/AudioReader.h
    sfizz/AudioSpan.h
    sfizz/Buffer.h
    sfizz/BufferPool.h
    sfizz/CCMap.h
    sfizz/Config.h
    sfizz/Curve.h
    sfizz/Debug.h
    sfizz/utility/NumericId.h
    sfizz/utility/SpinMutex.h
    sfizz/modulations/ModId.h
    sfizz/modulations/ModKey.h
    sfizz/modulations/ModKeyHash.h
    sfizz/modulations/ModMatrix.h
    sfizz/modulations/ModGenerator.h
    sfizz/modulations/sources/Controller.h
    sfizz/modulations/sources/LFO.h
    sfizz/effects/impl/ResonantArray.h
    sfizz/effects/impl/ResonantArrayAVX.h
    sfizz/effects/impl/ResonantArraySSE.h
    sfizz/effects/impl/ResonantString.h
    sfizz/effects/impl/ResonantStringAVX.h
    sfizz/effects/impl/ResonantStringSSE.h
    sfizz/effects/Apan.h
    sfizz/effects/CommonLFO.h
    sfizz/effects/CommonLFO.hpp
    sfizz/effects/Compressor.h
    sfizz/effects/Disto.h
    sfizz/effects/Eq.h
    sfizz/effects/Filter.h
    sfizz/effects/Fverb.h
    sfizz/effects/Gain.h
    sfizz/effects/Gate.h
    sfizz/effects/Limiter.h
    sfizz/effects/Lofi.h
    sfizz/effects/Nothing.h
    sfizz/effects/Rectify.h
    sfizz/effects/Strings.h
    sfizz/effects/Width.h
    sfizz/Effects.h
    sfizz/EGDescription.h
    sfizz/EQDescription.h
    sfizz/EQPool.h
    sfizz/FileId.h
    sfizz/FileMetadata.h
    sfizz/FilePool.h
    sfizz/FilterDescription.h
    sfizz/FilterPool.h
    sfizz/HistoricalBuffer.h
    sfizz/Interpolators.h
    sfizz/Interpolators.hpp
    sfizz/Logger.h
    sfizz/LFO.h
    sfizz/LFODescription.h
    sfizz/MathHelpers.h
    sfizz/MidiState.h
    sfizz/ModifierHelpers.h
    sfizz/OnePoleFilter.h
    sfizz/Oversampler.h
    sfizz/Panning.h
    sfizz/PolyphonyGroup.h
    sfizz/PowerFollower.h
    sfizz/railsback/2-1.h
    sfizz/railsback/4-1.h
    sfizz/railsback/4-2.h
    sfizz/Region.h
    sfizz/RegionSet.h
    sfizz/Resources.h
    sfizz/RTSemaphore.h
    sfizz/ScopedFTZ.h
    sfizz/SfzFilter.h
    sfizz/SfzFilterImpls.hpp
    sfizz/simd/Common.h
    sfizz/simd/HelpersAVX.h
    sfizz/simd/HelpersScalar.h
    sfizz/simd/HelpersSSE.h
    sfizz/SIMDConfig.h
    sfizz/SIMDHelpers.h
    sfizz/SisterVoiceRing.h
    sfizz/Smoothers.h
    sfizz/SwapAndPop.h
    sfizz/Synth.h
    sfizz/SynthConfig.h
    sfizz/Tuning.h
    sfizz/Voice.h
    sfizz/VoiceStealing.h
    sfizz/Wavetables.h
    sfizz.h
    sfizz.hpp)

set (SFIZZ_SOURCES
    sfizz/Synth.cpp
    sfizz/FileId.cpp
    sfizz/FilePool.cpp
    sfizz/FileMetadata.cpp
    sfizz/AudioReader.cpp
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
    sfizz/Curve.cpp
    sfizz/Smoothers.cpp
    sfizz/Wavetables.cpp
    sfizz/Tuning.cpp
    sfizz/RegionSet.cpp
    sfizz/PolyphonyGroup.cpp
    sfizz/VoiceStealing.cpp
    sfizz/RTSemaphore.cpp
    sfizz/Panning.cpp
    sfizz/Effects.cpp
    sfizz/LFO.cpp
    sfizz/LFODescription.cpp
    sfizz/PowerFollower.cpp
    sfizz/modulations/ModId.cpp
    sfizz/modulations/ModKey.cpp
    sfizz/modulations/ModKeyHash.cpp
    sfizz/modulations/ModMatrix.cpp
    sfizz/modulations/sources/Controller.cpp
    sfizz/modulations/sources/LFO.cpp
    sfizz/utility/SpinMutex.cpp
    sfizz/effects/Nothing.cpp
    sfizz/effects/Filter.cpp
    sfizz/effects/Eq.cpp
    sfizz/effects/Apan.cpp
    sfizz/effects/Lofi.cpp
    sfizz/effects/Limiter.cpp
    sfizz/effects/Compressor.cpp
    sfizz/effects/Gate.cpp
    sfizz/effects/Disto.cpp
    sfizz/effects/Strings.cpp
    sfizz/effects/Fverb.cpp
    sfizz/effects/Rectify.cpp
    sfizz/effects/Gain.cpp
    sfizz/effects/Width.cpp
    sfizz/effects/impl/ResonantString.cpp
    sfizz/effects/impl/ResonantStringSSE.cpp
    sfizz/effects/impl/ResonantStringAVX.cpp
    sfizz/effects/impl/ResonantArray.cpp
    sfizz/effects/impl/ResonantArraySSE.cpp
    sfizz/effects/impl/ResonantArrayAVX.cpp)

include (SfizzSIMDSourceFiles)
sfizz_add_simd_sources (SFIZZ_SOURCES ".")

# Parser core library
set (SFIZZ_PARSER_HEADERS
    sfizz/Defaults.h
    sfizz/LeakDetector.h
    sfizz/Range.h
    sfizz/Opcode.h
    sfizz/Macros.h
    sfizz/Parser.h
    sfizz/parser/Parser.h
    sfizz/parser/ParserPrivate.h
    sfizz/parser/ParserPrivate.hpp
    sfizz/SfzHelpers.h
    sfizz/StringViewHelpers.h)

set (SFIZZ_PARSER_SOURCES
    sfizz/Parser.cpp
    sfizz/Opcode.cpp
    sfizz/OpcodeCleanup.cpp
    sfizz/SfzHelpers.cpp
    sfizz/parser/Parser.cpp
    sfizz/parser/ParserPrivate.cpp)

set (SFIZZ_PARSER_OTHER sfizz/OpcodeCleanup.re)
source_group ("Other Files" FILES ${SFIZZ_PARSER_OTHER})

add_library (sfizz_parser STATIC)
target_sources (sfizz_parser PRIVATE
    ${SFIZZ_PARSER_HEADERS} ${SFIZZ_PARSER_SOURCES} ${SFIZZ_PARSER_OTHER})
target_include_directories (sfizz_parser PUBLIC sfizz)
target_include_directories (sfizz_parser PUBLIC external)
target_link_libraries (sfizz_parser PUBLIC absl::strings PRIVATE absl::flat_hash_map)

# Sfizz static library
add_library(sfizz_static STATIC)
target_sources(sfizz_static PRIVATE
    ${SFIZZ_HEADERS} ${SFIZZ_SOURCES} ${FAUST_FILES} sfizz/sfizz_wrapper.cpp sfizz/sfizz.cpp)
target_include_directories (sfizz_static PUBLIC .)
target_include_directories (sfizz_static PUBLIC external)
target_link_libraries (sfizz_static PUBLIC absl::strings absl::span)
target_link_libraries (sfizz_static PRIVATE sfizz_parser absl::flat_hash_map Threads::Threads sfizz-sndfile sfizz-pugixml sfizz-spline sfizz-tunings sfizz-kissfft sfizz-cpuid sfizz-atomic)
set_target_properties (sfizz_static PROPERTIES OUTPUT_NAME sfizz PUBLIC_HEADER "sfizz.h;sfizz.hpp")
if (WIN32)
    target_compile_definitions (sfizz_static PRIVATE _USE_MATH_DEFINES)
endif()
if (SFIZZ_RELEASE_ASSERTS)
    target_compile_definitions (sfizz_static PRIVATE "SFIZZ_ENABLE_RELEASE_ASSERT=1")
endif()
sfizz_enable_fast_math(sfizz_static)

if (NOT MSVC)
    install (TARGETS sfizz_static
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT "development")

    configure_file (${PROJECT_SOURCE_DIR}/scripts/sfizz.pc.in sfizz.pc @ONLY)
    install (FILES ${CMAKE_BINARY_DIR}/src/sfizz.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
        COMPONENT "development")
endif()
if(WIN32)
    include(VSTConfig)
    configure_file (${PROJECT_SOURCE_DIR}/scripts/innosetup.iss.in ${PROJECT_BINARY_DIR}/innosetup.iss @ONLY)
endif()

configure_file (${PROJECT_SOURCE_DIR}/doxygen/scripts/Doxyfile.in ${PROJECT_SOURCE_DIR}/Doxyfile @ONLY)

add_library (sfizz::parser ALIAS sfizz_parser)
add_library (sfizz::sfizz ALIAS sfizz_static)

# Shared library and installation target
if (SFIZZ_SHARED)
    add_library (sfizz_shared SHARED)
    target_sources(sfizz_shared PRIVATE
        ${SFIZZ_HEADERS} ${SFIZZ_SOURCES} ${FAUST_FILES} sfizz/sfizz_wrapper.cpp sfizz/sfizz.cpp)
    target_include_directories (sfizz_shared PRIVATE .)
    target_include_directories (sfizz_shared PRIVATE external)
    target_link_libraries (sfizz_shared PRIVATE absl::strings absl::span sfizz_parser absl::flat_hash_map Threads::Threads sfizz-sndfile sfizz-pugixml sfizz-spline sfizz-tunings sfizz-kissfft sfizz-cpuid sfizz-atomic)
    if (WIN32)
        target_compile_definitions (sfizz_shared PRIVATE _USE_MATH_DEFINES)
    endif()
    if (SFIZZ_RELEASE_ASSERTS)
        target_compile_definitions (sfizz_shared PRIVATE "SFIZZ_ENABLE_RELEASE_ASSERT=1")
    endif()
    target_compile_definitions(sfizz_shared PRIVATE SFIZZ_EXPORT_SYMBOLS)
    set_target_properties (sfizz_shared PROPERTIES SOVERSION ${PROJECT_VERSION_MAJOR} OUTPUT_NAME sfizz)
    sfizz_enable_lto_if_needed(sfizz_shared)
    sfizz_enable_fast_math(sfizz_shared)

    if (NOT MSVC)
        install (TARGETS sfizz_shared
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            COMPONENT "runtime")
    endif()
endif()
