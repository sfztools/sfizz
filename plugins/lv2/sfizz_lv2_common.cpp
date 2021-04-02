// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz_lv2.h"
#include "sfizz_lv2_plugin.h"

bool sfizz_lv2_fetch_description(
    sfizz_plugin_t *self, const int *serial,
    uint8_t **descp, uint32_t *sizep, int *serialp)
{
    // must be thread-safe

    if (serial && self->sfz_blob_serial == *serial)
        return false;

    self->sfz_blob_mutex->lock();
    uint32_t size = self->sfz_blob_size;
    uint8_t *data = new uint8_t[size];
    int new_serial = self->sfz_blob_serial;
    memcpy(data, self->sfz_blob_data, size);
    self->sfz_blob_mutex->unlock();

    *descp = data;
    *sizep = size;
    *serialp = new_serial;

    return true;
}
