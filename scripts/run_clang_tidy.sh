#!/bin/sh

clang-tidy \
  src/sfizz/ADSREnvelope.cpp \
  src/sfizz/Curve.cpp \
  src/sfizz/Effects.cpp \
  src/sfizz/EQPool.cpp \
  src/sfizz/FilePool.cpp \
  src/sfizz/FilterPool.cpp \
  src/sfizz/FloatEnvelopes.cpp \
  src/sfizz/Logger.cpp \
  src/sfizz/MidiState.cpp \
  src/sfizz/Opcode.cpp \
  src/sfizz/Oversampler.cpp \
  src/sfizz/Parser.cpp \
  src/sfizz/Panning.cpp \
  src/sfizz/sfizz.cpp \
  src/sfizz/Region.cpp \
  src/sfizz/SfzHelpers.cpp \
  src/sfizz/SIMDHelpers.cpp \
  src/sfizz/simd/HelpersSSE.cpp \
  src/sfizz/simd/HelpersAVX.cpp \
  src/sfizz/Synth.cpp \
  src/sfizz/Voice.cpp \
  src/sfizz/effects/Eq.cpp \
  src/sfizz/effects/Filter.cpp \
  src/sfizz/effects/Lofi.cpp \
  src/sfizz/effects/Nothing.cpp \
  vst/SfizzVstController.cpp \
  vst/SfizzVstProcessor.cpp \
  vst/SfizzVstEditor.cpp \
  vst/SfizzVstState.cpp \
  -- -Iexternal/abseil-cpp -Isrc/external -Isrc/external/pugixml/src \
      -Isrc/sfizz -Isrc -Isrc/external/spline -Isrc/external/cpuid/src \
      -Ivst -Ivst/external/VST_SDK/VST3_SDK -Ivst/external/VST_SDK/VST3_SDK/vstgui4 -Ivst/external/ring_buffer -DNDEBUG
