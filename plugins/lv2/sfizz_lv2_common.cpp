// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sfizz_lv2.h"
#include "sfizz_lv2_plugin.h"
#include "sfizz/Config.h"

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

struct sfizz_lv2_ccmap {
    LV2_URID *cc_to_urid;
    int *urid_to_cc;
    LV2_URID min_cc_urid;
    LV2_URID max_cc_urid;
};

sfizz_lv2_ccmap *
sfizz_lv2_ccmap_create(LV2_URID_Map* map)
{
    // store the associations between CC number and parameter URID
    // construct it trivially in the form of a tabulated perfect hash function

    LV2_URID* cc_to_urid = new LV2_URID[sfz::config::numCCs];
    int* urid_to_cc;
    int urid_to_cc_size;

    LV2_URID min_cc_urid {};
    LV2_URID max_cc_urid {};
    for (int cc = 0; cc < sfz::config::numCCs; ++cc) {
        char name[256];
        sprintf(name, SFIZZ_URI "#cc%03d", cc);
        LV2_URID urid = map->map(map->handle, name);
        if (cc == 0) {
            min_cc_urid = urid;
            max_cc_urid = urid;
        }
        else {
            min_cc_urid = (urid < min_cc_urid) ? urid : min_cc_urid;
            max_cc_urid = (urid > max_cc_urid) ? urid : max_cc_urid;
        }
        cc_to_urid[cc] = urid;
    }

    urid_to_cc_size = max_cc_urid - min_cc_urid + 1;
    urid_to_cc = new int[urid_to_cc_size];

    for (int i = 0; i < urid_to_cc_size; ++i)
        urid_to_cc[i] = -1;

    for (int cc = 0; cc < sfz::config::numCCs; ++cc) {
        LV2_URID urid = cc_to_urid[cc];
        urid_to_cc[urid - min_cc_urid] = cc;
    }

    sfizz_lv2_ccmap *self = new sfizz_lv2_ccmap;
    self->cc_to_urid = cc_to_urid;
    self->urid_to_cc = urid_to_cc;
    self->min_cc_urid = min_cc_urid;
    self->max_cc_urid = max_cc_urid;
    return self;
}

void sfizz_lv2_ccmap_free(sfizz_lv2_ccmap *ccmap)
{
    if (ccmap) {
        delete[] ccmap->cc_to_urid;
        delete[] ccmap->urid_to_cc;
        delete ccmap;
    }
}

LV2_URID sfizz_lv2_ccmap_map(const sfizz_lv2_ccmap *ccmap, int cc)
{
    LV2_URID urid = 0;
    if (cc >= 0 && cc < sfz::config::numCCs)
        urid = ccmap->cc_to_urid[cc];
    return urid;
}

int sfizz_lv2_ccmap_unmap(const sfizz_lv2_ccmap *ccmap, LV2_URID urid)
{
    int cc = -1;
    if (urid >= ccmap->min_cc_urid && urid <= ccmap->max_cc_urid)
        cc = ccmap->urid_to_cc[urid - ccmap->min_cc_urid];
    return cc;
}
