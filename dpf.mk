#
# A build file to help using sfizz with the DISTRHO Plugin Framework (DPF)
# ------------------------------------------------------------------------
#
# Usage notes:
#
# 1. Start with the template DPF project
#
#    (for example https://github.com/SpotlightKid/cookiecutter-dpf-effect)
#
# 2. In the top directory of the DPF project, edit `Makefile`
#
#    Add the following line, after the `all` rule definition:
#
#        include <path-to-sfizz>/dpf.mk
#
#    Add `sfizz-lib` to the `libs` rule definition:
#
#        libs: sfizz-lib
#
#    If you want you can add `sfizz-clean` to the `clean` rule.
#
#        clean: sfizz-clean
#
# 3. In the Makefile of your plugin folder, eg. `plugins/MyPlugin/Makefile`
#
#    Add the following line, after `include ../../dpf/Makefile.plugins.mk`
#
#        include <path-to-sfizz>/dpf.mk
#        BUILD_C_FLAGS += $(SFIZZ_C_FLAGS)
#        BUILD_CXX_FLAGS += $(SFIZZ_CXX_FLAGS)
#        LINK_FLAGS += $(SFIZZ_LINK_FLAGS)
#
# 4. Build using `make` from the top folder.
#

SFIZZ_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
SFIZZ_BUILD_DIR := $(SFIZZ_DIR)/dpf-build

SFIZZ_C_FLAGS = -I$(SFIZZ_DIR)/src
SFIZZ_CXX_FLAGS = $(SFIZZ_C_FLAGS)
SFIZZ_LINK_FLAGS = $(SFIZZ_BUILD_DIR)/libsfizz.a

ifeq ($(LINUX),true)
SFIZZ_LINK_FLAGS += -pthread
endif

sfizz-all: sfizz-lib

sfizz-lib: $(SFIZZ_BUILD_DIR)/libsfizz.a

sfizz-clean:
	rm -rf $(SFIZZ_BUILD_DIR)

.PHONY: sfizz-all sfizz-lib sfizz-clean

SFIZZ_SOURCES = \
	src/sfizz/ADSREnvelope.cpp \
	src/sfizz/Curve.cpp \
	src/sfizz/effects/Apan.cpp \
	src/sfizz/Effects.cpp \
	src/sfizz/effects/Eq.cpp \
	src/sfizz/effects/Filter.cpp \
	src/sfizz/effects/Gain.cpp \
	src/sfizz/effects/impl/ResonantArrayAVX.cpp \
	src/sfizz/effects/impl/ResonantArray.cpp \
	src/sfizz/effects/impl/ResonantArraySSE.cpp \
	src/sfizz/effects/impl/ResonantStringAVX.cpp \
	src/sfizz/effects/impl/ResonantString.cpp \
	src/sfizz/effects/impl/ResonantStringSSE.cpp \
	src/sfizz/effects/Limiter.cpp \
	src/sfizz/effects/Lofi.cpp \
	src/sfizz/effects/Nothing.cpp \
	src/sfizz/effects/Rectify.cpp \
	src/sfizz/effects/Strings.cpp \
	src/sfizz/effects/Width.cpp \
	src/sfizz/EQPool.cpp \
	src/sfizz/FileId.cpp \
	src/sfizz/FilePool.cpp \
	src/sfizz/FilterPool.cpp \
	src/sfizz/FloatEnvelopes.cpp \
	src/sfizz/Logger.cpp \
	src/sfizz/MidiState.cpp \
	src/sfizz/OpcodeCleanup.cpp \
	src/sfizz/Opcode.cpp \
	src/sfizz/Oversampler.cpp \
	src/sfizz/Parser.cpp \
	src/sfizz/parser/Parser.cpp \
	src/sfizz/parser/ParserPrivate.cpp \
	src/sfizz/Region.cpp \
	src/sfizz/RTSemaphore.cpp \
	src/sfizz/ScopedFTZ.cpp \
	src/sfizz/sfizz.cpp \
	src/sfizz/sfizz_wrapper.cpp \
	src/sfizz/SfzFilter.cpp \
	src/sfizz/SfzHelpers.cpp \
	src/sfizz/SIMDDummy.cpp \
	src/sfizz/SIMDNEON.cpp \
	src/sfizz/SIMDSSE.cpp \
	src/sfizz/Synth.cpp \
	src/sfizz/Voice.cpp \
	src/sfizz/Wavetables.cpp

### Other internal

SFIZZ_C_FLAGS += -I$(SFIZZ_DIR)/src/sfizz
SFIZZ_C_FLAGS += -I$(SFIZZ_DIR)/src/external

# Sndfile dependency

SFIZZ_C_FLAGS += $(shell $(PKG_CONFIG) --cflags sndfile)
SFIZZ_LINK_FLAGS += $(shell $(PKG_CONFIG) --libs sndfile)

### Abseil dependency

SFIZZ_C_FLAGS += -I$(SFIZZ_DIR)/external/abseil-cpp
# absl::base
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/base/internal/cycleclock.cc \
	external/abseil-cpp/absl/base/internal/spinlock.cc \
	external/abseil-cpp/absl/base/internal/sysinfo.cc \
	external/abseil-cpp/absl/base/internal/thread_identity.cc \
	external/abseil-cpp/absl/base/internal/unscaledcycleclock.cc
# absl::malloc_internal
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/base/internal/low_level_alloc.cc
# absl::raw_logging_internal
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/base/internal/raw_logging.cc
# absl::spinlock_wait
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/base/internal/spinlock_wait.cc
# absl::throw_delegate
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/base/internal/throw_delegate.cc
# absl::strings
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/strings/ascii.cc \
	external/abseil-cpp/absl/strings/charconv.cc \
	external/abseil-cpp/absl/strings/escaping.cc \
	external/abseil-cpp/absl/strings/internal/charconv_bigint.cc \
	external/abseil-cpp/absl/strings/internal/charconv_parse.cc \
	external/abseil-cpp/absl/strings/internal/memutil.cc \
	external/abseil-cpp/absl/strings/match.cc \
	external/abseil-cpp/absl/strings/numbers.cc \
	external/abseil-cpp/absl/strings/str_cat.cc \
	external/abseil-cpp/absl/strings/str_replace.cc \
	external/abseil-cpp/absl/strings/str_split.cc \
	external/abseil-cpp/absl/strings/string_view.cc \
	external/abseil-cpp/absl/strings/substitute.cc
# absl::hashtablez_sampler
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/container/internal/hashtablez_sampler.cc \
	external/abseil-cpp/absl/container/internal/hashtablez_sampler_force_weak_definition.cc
# absl::synchronization
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/synchronization/barrier.cc \
	external/abseil-cpp/absl/synchronization/blocking_counter.cc \
	external/abseil-cpp/absl/synchronization/internal/create_thread_identity.cc \
	external/abseil-cpp/absl/synchronization/internal/per_thread_sem.cc \
	external/abseil-cpp/absl/synchronization/internal/waiter.cc \
	external/abseil-cpp/absl/synchronization/notification.cc \
	external/abseil-cpp/absl/synchronization/mutex.cc
# absl::time
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/time/civil_time.cc \
	external/abseil-cpp/absl/time/clock.cc \
	external/abseil-cpp/absl/time/duration.cc \
	external/abseil-cpp/absl/time/format.cc \
	external/abseil-cpp/absl/time/time.cc
# absl::stacktrace
SFIZZ_SOURCES += \
	external/abseil-cpp/absl/debugging/stacktrace.cc

### Spline dependency

SFIZZ_C_FLAGS += -I$(SFIZZ_DIR)/src/external/spline
SFIZZ_SOURCES += src/external/spline/spline/spline.cpp

### Cpuid dependency

SFIZZ_C_FLAGS += \
	-I$(SFIZZ_DIR)/src/external/cpuid/src \
	-I$(SFIZZ_DIR)/src/external/cpuid/platform/src
SFIZZ_SOURCES += \
	src/external/cpuid/src/cpuid/cpuinfo.cpp \
	src/external/cpuid/src/cpuid/version.cpp

### Pugixml dependency

SFIZZ_C_FLAGS += -I$(SFIZZ_DIR)/src/external/pugixml/src
SFIZZ_SOURCES += src/external/pugixml/src/pugixml.cpp

### Kissfft dependency

SFIZZ_C_FLAGS += \
	-I$(SFIZZ_DIR)/src/external/kiss_fft \
	-I$(SFIZZ_DIR)/src/external/kiss_fft/tools
SFIZZ_SOURCES += \
	src/external/kiss_fft/kiss_fft.c \
	src/external/kiss_fft/tools/kiss_fftr.c

###

SFIZZ_OBJECTS = $(SFIZZ_SOURCES:%=$(SFIZZ_BUILD_DIR)/%.o)

$(SFIZZ_BUILD_DIR)/libsfizz.a: $(SFIZZ_OBJECTS)
	-@mkdir -p $(dir $@)
	@echo "Creating $@"
	$(SILENT)$(AR) crs $@ $^

###

ifeq ($(CPU_I386_OR_X86_64),true)

$(SFIZZ_BUILD_DIR)/%SSE.cpp.o: $(SFIZZ_DIR)/%SSE.cpp
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $(BUILD_CXX_FLAGS) $(SFIZZ_CXX_FLAGS) -msse -c -o $@ $<

$(SFIZZ_BUILD_DIR)/%AVX.cpp.o: $(SFIZZ_DIR)/%AVX.cpp
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $(BUILD_CXX_FLAGS) $(SFIZZ_CXX_FLAGS) -mavx -c -o $@ $<

endif

###

$(SFIZZ_BUILD_DIR)/%.cpp.o: $(SFIZZ_DIR)/%.cpp
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $(BUILD_CXX_FLAGS) $(SFIZZ_CXX_FLAGS) -c -o $@ $<

$(SFIZZ_BUILD_DIR)/%.cc.o: $(SFIZZ_DIR)/%.cc
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $(BUILD_CXX_FLAGS) $(SFIZZ_CXX_FLAGS) -c -o $@ $<

$(SFIZZ_BUILD_DIR)/%.c.o: $(SFIZZ_DIR)/%.c
	-@mkdir -p $(dir $@)
	@echo "Compiling $<"
	$(SILENT)$(CC) $(BUILD_C_FLAGS) $(SFIZZ_C_FLAGS) -c -o $@ $<

-include $(SFIZZ_OBJECTS:%.o=%.d)
