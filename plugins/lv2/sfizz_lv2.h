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

#define SFIZZ_URI "http://sfztools.github.io/sfizz"
#define SFIZZ_UI_URI "http://sfztools.github.io/sfizz#ui"
#define SFIZZ_PREFIX SFIZZ_URI "#"
#define SFIZZ__sfzFile SFIZZ_URI ":" "sfzfile"
#define SFIZZ__tuningfile SFIZZ_URI ":" "tuningfile"
#define SFIZZ__numVoices SFIZZ_URI ":" "numvoices"
#define SFIZZ__preloadSize SFIZZ_URI ":" "preload_size"
#define SFIZZ__oversampling SFIZZ_URI ":" "oversampling"
// These ones are just for the worker
#define SFIZZ__logStatus SFIZZ_URI ":" "log_status"
#define SFIZZ__checkModification SFIZZ_URI ":" "check_modification"
// OSC atoms
#define SFIZZ__OSCBlob SFIZZ_URI ":" "OSCBlob"

enum
{
    SFIZZ_CONTROL = 0,
    SFIZZ_NOTIFY = 1,
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
    SFIZZ_ACTIVE_VOICES = 12,
    SFIZZ_NUM_CURVES = 13,
    SFIZZ_NUM_MASTERS = 14,
    SFIZZ_NUM_GROUPS = 15,
    SFIZZ_NUM_REGIONS = 16,
    SFIZZ_NUM_SAMPLES = 17,
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
