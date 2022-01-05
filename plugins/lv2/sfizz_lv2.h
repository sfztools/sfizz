// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include <lv2/atom/atom.h>
#include <lv2/atom/forge.h>
#include <lv2/atom/util.h>
#include <lv2/buf-size/buf-size.h>
#include <lv2/core/lv2.h>
#include <lv2/core/lv2_util.h>
#include <lv2/midi/midi.h>
#include <lv2/options/options.h>
#include <lv2/parameters/parameters.h>
#include <lv2/patch/patch.h>
#include <lv2/state/state.h>
#include <lv2/urid/urid.h>
#include <lv2/worker/worker.h>
#include <lv2/log/logger.h>
#include <lv2/log/log.h>
#include <lv2/time/time.h>

#include <ardour/lv2_extensions.h>

#include <stdint.h>

#define MAX_PATH_SIZE 1024
#define ATOM_TEMP_SIZE 8192
#define OSC_TEMP_SIZE 8192
#define MULTI_OUTPUT_COUNT 16

#define SFIZZ_URI "http://sfztools.github.io/sfizz"
#define SFIZZ_MULTI_URI SFIZZ_URI "-multi"
#define SFIZZ_UI_URI "http://sfztools.github.io/sfizz#ui"
#define SFIZZ_PREFIX SFIZZ_URI "#"
#define SFIZZ__sfzFile SFIZZ_URI ":" "sfzfile"
#define SFIZZ__tuningfile SFIZZ_URI ":" "tuningfile"
#define SFIZZ__numVoices SFIZZ_URI ":" "numvoices"
#define SFIZZ__preloadSize SFIZZ_URI ":" "preload_size"
#define SFIZZ__oversampling SFIZZ_URI ":" "oversampling"
#define SFIZZ__lastKeyswitch SFIZZ_URI ":" "last_keyswitch"
#define SFIZZ__description SFIZZ_URI ":" "description"
// These ones are just for the worker
#define SFIZZ__logStatus SFIZZ_URI ":" "log_status"
#define SFIZZ__checkModification SFIZZ_URI ":" "check_modification"
// OSC atoms
#define SFIZZ__OSCBlob SFIZZ_URI ":" "OSCBlob"
#define SFIZZ__Notify SFIZZ_URI ":" "Notify"
// Level atoms
#define SFIZZ__AudioLevel SFIZZ_URI ":" "AudioLevel"

enum
{
    SFIZZ_CONTROL = 0,
    SFIZZ_AUTOMATE = 1,
    SFIZZ_LEFT = 2,
    SFIZZ_RIGHT = 3,
    SFIZZ_VOLUME = 4,
    SFIZZ_POLYPHONY = 5,
    SFIZZ_OVERSAMPLING = 6,
    SFIZZ_PRELOAD = 7,
    SFIZZ_FREEWHEELING = 8,
    SFIZZ_SCALA_ROOT_KEY = 9,
    SFIZZ_TUNING_FREQUENCY = 10,
    SFIZZ_STRETCH_TUNING = 11,
    SFIZZ_SAMPLE_QUALITY = 12,
    SFIZZ_OSCILLATOR_QUALITY = 13,
    SFIZZ_ACTIVE_VOICES = 14,
    SFIZZ_NUM_CURVES = 15,
    SFIZZ_NUM_MASTERS = 16,
    SFIZZ_NUM_GROUPS = 17,
    SFIZZ_NUM_REGIONS = 18,
    SFIZZ_NUM_SAMPLES = 19,
    SFIZZ_FREEWHEELING_SAMPLE_QUALITY = 20,
    SFIZZ_FREEWHEELING_OSCILLATOR_QUALITY = 21,
    SFIZZ_SUSTAIN_CANCELS_RELEASE = 22,
};

enum
{
    SFIZZ_MULTI_CONTROL = 0,
    SFIZZ_MULTI_AUTOMATE = 1,
    SFIZZ_MULTI_OUT1L = 2,
    SFIZZ_MULTI_OUT1R = 3,
    SFIZZ_MULTI_OUT2L = 4,
    SFIZZ_MULTI_OUT2R = 5,
    SFIZZ_MULTI_OUT3L = 6,
    SFIZZ_MULTI_OUT3R = 7,
    SFIZZ_MULTI_OUT4L = 8,
    SFIZZ_MULTI_OUT4R = 9,
    SFIZZ_MULTI_OUT5L = 10,
    SFIZZ_MULTI_OUT5R = 11,
    SFIZZ_MULTI_OUT6L = 12,
    SFIZZ_MULTI_OUT6R = 13,
    SFIZZ_MULTI_OUT7L = 14,
    SFIZZ_MULTI_OUT7R = 15,
    SFIZZ_MULTI_OUT8L = 16,
    SFIZZ_MULTI_OUT8R = 17,
    SFIZZ_MULTI_VOLUME = 18,
    SFIZZ_MULTI_POLYPHONY = 19,
    SFIZZ_MULTI_OVERSAMPLING = 20,
    SFIZZ_MULTI_PRELOAD = 21,
    SFIZZ_MULTI_FREEWHEELING = 22,
    SFIZZ_MULTI_SCALA_ROOT_KEY = 23,
    SFIZZ_MULTI_TUNING_FREQUENCY = 24,
    SFIZZ_MULTI_STRETCH_TUNING = 25,
    SFIZZ_MULTI_SAMPLE_QUALITY = 26,
    SFIZZ_MULTI_OSCILLATOR_QUALITY = 27,
    SFIZZ_MULTI_ACTIVE_VOICES = 28,
    SFIZZ_MULTI_NUM_CURVES = 29,
    SFIZZ_MULTI_NUM_MASTERS = 30,
    SFIZZ_MULTI_NUM_GROUPS = 31,
    SFIZZ_MULTI_NUM_REGIONS = 32,
    SFIZZ_MULTI_NUM_SAMPLES = 33,
    SFIZZ_MULTI_FREEWHEELING_SAMPLE_QUALITY = 34,
    SFIZZ_MULTI_FREEWHEELING_OSCILLATOR_QUALITY = 35,
    SFIZZ_MULTI_SUSTAIN_CANCELS_RELEASE = 36,
};

// For use with instance-access
struct sfizz_plugin_t;

/**
 * @brief Fetch a copy of the current description, if more recent than what the
 *        version we already have.
 *
 * @param self     The LV2 plugin
 * @param serial   The optional serial number to compare against
 * @param desc     The memory zone which receives the copy
 * @param sizep    The memory zone which receives the size
 * @param serialp  The memory zone which receives the new serial
 * @return if serial is not null and its value isn't older, false
 *         otherwise, true, and the pointer returned in descp must be freed with delete[] after use
 */
bool sfizz_lv2_fetch_description(
    sfizz_plugin_t *self, const int *serial,
    uint8_t **descp, uint32_t *sizep, int *serialp);

/**
 * @brief Returns the number of outputs of the plugin
 *
 * @param self
 * @return int
 */
int sfizz_lv2_get_num_outputs(sfizz_plugin_t *self);

#if defined(SFIZZ_LV2_UI)
void sfizz_lv2_set_ui_active(sfizz_plugin_t *self, bool ui_active);
#endif

// Mapping URID to CC and vice-versa
struct sfizz_lv2_ccmap;
sfizz_lv2_ccmap *sfizz_lv2_ccmap_create(LV2_URID_Map* map);
void sfizz_lv2_ccmap_free(sfizz_lv2_ccmap *ccmap);
LV2_URID sfizz_lv2_ccmap_map(const sfizz_lv2_ccmap *ccmap, int cc);
int sfizz_lv2_ccmap_unmap(const sfizz_lv2_ccmap *ccmap, LV2_URID urid);

struct sfizz_lv2_ccmap_delete {
    void operator()(sfizz_lv2_ccmap* ccmap) const noexcept { sfizz_lv2_ccmap_free(ccmap); }
};
