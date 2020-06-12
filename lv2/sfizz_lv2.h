// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#define SFIZZ_URI "http://sfztools.github.io/sfizz"
#define SFIZZ_PREFIX SFIZZ_URI "#"
#define SFIZZ__sfzFile "http://sfztools.github.io/sfizz:sfzfile"
#define SFIZZ__tuningfile "http://sfztools.github.io/sfizz:tuningfile"
#define SFIZZ__numVoices "http://sfztools.github.io/sfizz:numvoices"
#define SFIZZ__preloadSize "http://sfztools.github.io/sfizz:preload_size"
#define SFIZZ__oversampling "http://sfztools.github.io/sfizz:oversampling"
// These ones are just for the worker
#define SFIZZ__logStatus "http://sfztools.github.io/sfizz:log_status"
#define SFIZZ__checkModification "http://sfztools.github.io/sfizz:check_modification"

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
};
