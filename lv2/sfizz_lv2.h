// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#define MAX_PATH_SIZE 1024

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
#define SFIZZ__controllerChange SFIZZ_URI ":" "controller_change"

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
