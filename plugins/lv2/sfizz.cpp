/*
  SPDX-License-Identifier: ISC

  Sfizz LV2 plugin

  Copyright 2019-2020, Paul Ferrand <paul@ferrand.cc>

  This file was based on skeleton and example code from the LV2 plugin
  distribution available at http://lv2plug.in/

  The LV2 sample plugins have the following copyright and notice, which are
  extended to the current work:
  Copyright 2011-2016 David Robillard <d@drobilla.net>
  Copyright 2011 Gabriel M. Beddingfield <gabriel@teuton.org>
  Copyright 2011 James Morris <jwm.art.net@gmail.com>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  Compiling this plugin statically against libsndfile implies distributing it
  under the terms of the LGPL v3 license. See the LICENSE.md file for more
  information. If you did not receive a LICENSE.md file, inform the current
  maintainer.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "sfizz_lv2.h"
#include "sfizz_lv2_plugin.h"

#include "sfizz/import/ForeignInstrument.h"
#include "plugin/InstrumentDescription.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <memory>

#define CHANNEL_MASK 0x0F
#define MIDI_CHANNEL(byte) (byte & CHANNEL_MASK)
#define MIDI_STATUS(byte) (byte & ~CHANNEL_MASK)
#define PITCH_BUILD_AND_CENTER(first_byte, last_byte) (int)(((unsigned int)last_byte << 7) + (unsigned int)first_byte) - 8192
#define MAX_BLOCK_SIZE 8192
#define MAX_VOICES 256
#define DEFAULT_VOICES 64
#define DEFAULT_OVERSAMPLING SFIZZ_OVERSAMPLING_X1
#define DEFAULT_PRELOAD 8192
#define LOG_SAMPLE_COUNT 48000
#define UNUSED(x) (void)(x)

#ifndef NDEBUG
#define LV2_DEBUG(...) lv2_log_note(&self->logger, "[DEBUG] " __VA_ARGS__)
#else
#define LV2_DEBUG(...)
#endif

enum
{
    SFIZZ_TIMEINFO_POSITION = 1 << 0,
    SFIZZ_TIMEINFO_SIGNATURE = 1 << 1,
    SFIZZ_TIMEINFO_TEMPO = 1 << 2,
    SFIZZ_TIMEINFO_SPEED = 1 << 3,
};

///
static bool
sfizz_lv2_load_file(sfizz_plugin_t *self, const char *file_path);

static bool
sfizz_lv2_load_scala_file(sfizz_plugin_t *self, const char *file_path);

///
static void
sfizz_lv2_state_free_path(LV2_State_Free_Path_Handle handle,
                          char *path)
{
    (void)handle;
    free(path);
}

static LV2_State_Free_Path sfizz_State_Free_Path =
{
    NULL,
    &sfizz_lv2_state_free_path,
};

static void
sfizz_lv2_map_required_uris(sfizz_plugin_t *self)
{
    LV2_URID_Map *map = self->map;
    self->midi_event_uri = map->map(map->handle, LV2_MIDI__MidiEvent);
    self->max_block_length_uri = map->map(map->handle, LV2_BUF_SIZE__maxBlockLength);
    self->nominal_block_length_uri = map->map(map->handle, LV2_BUF_SIZE__nominalBlockLength);
    self->sample_rate_uri = map->map(map->handle, LV2_PARAMETERS__sampleRate);
    self->atom_float_uri = map->map(map->handle, LV2_ATOM__Float);
    self->atom_double_uri = map->map(map->handle, LV2_ATOM__Double);
    self->atom_int_uri = map->map(map->handle, LV2_ATOM__Int);
    self->atom_long_uri = map->map(map->handle, LV2_ATOM__Long);
    self->atom_path_uri = map->map(map->handle, LV2_ATOM__Path);
    self->atom_urid_uri = map->map(map->handle, LV2_ATOM__URID);
    self->atom_object_uri = map->map(map->handle, LV2_ATOM__Object);
    self->atom_blank_uri = map->map(map->handle, LV2_ATOM__Blank);
    self->patch_set_uri = map->map(map->handle, LV2_PATCH__Set);
    self->patch_get_uri = map->map(map->handle, LV2_PATCH__Get);
    self->patch_put_uri = map->map(map->handle, LV2_PATCH__Put);
    self->patch_body_uri = map->map(map->handle, LV2_PATCH__body);
    self->patch_property_uri = map->map(map->handle, LV2_PATCH__property);
    self->patch_value_uri = map->map(map->handle, LV2_PATCH__value);
    self->state_changed_uri = map->map(map->handle, LV2_STATE__StateChanged);
    self->sfizz_sfz_file_uri = map->map(map->handle, SFIZZ__sfzFile);
    self->sfizz_scala_file_uri = map->map(map->handle, SFIZZ__tuningfile);
    self->sfizz_num_voices_uri = map->map(map->handle, SFIZZ__numVoices);
    self->sfizz_preload_size_uri = map->map(map->handle, SFIZZ__preloadSize);
    self->sfizz_oversampling_uri = map->map(map->handle, SFIZZ__oversampling);
    self->sfizz_log_status_uri = map->map(map->handle, SFIZZ__logStatus);
    self->sfizz_log_status_uri = map->map(map->handle, SFIZZ__logStatus);
    self->sfizz_check_modification_uri = map->map(map->handle, SFIZZ__checkModification);
    self->sfizz_osc_blob_uri = map->map(map->handle, SFIZZ__OSCBlob);
    self->time_position_uri = map->map(map->handle, LV2_TIME__Position);
    self->time_bar_uri = map->map(map->handle, LV2_TIME__bar);
    self->time_bar_beat_uri = map->map(map->handle, LV2_TIME__barBeat);
    self->time_beat_unit_uri = map->map(map->handle, LV2_TIME__beatUnit);
    self->time_beats_per_bar_uri = map->map(map->handle, LV2_TIME__beatsPerBar);
    self->time_beats_per_minute_uri = map->map(map->handle, LV2_TIME__beatsPerMinute);
    self->time_speed_uri = map->map(map->handle, LV2_TIME__speed);
}

static bool
sfizz_atom_extract_real(sfizz_plugin_t *self, const LV2_Atom *atom, double *real)
{
    if (!atom)
        return false;

    const LV2_URID type = atom->type;

    if (type == self->atom_int_uri && atom->size >= sizeof(int32_t)) {
        *real = ((const LV2_Atom_Int *)atom)->body;
        return true;
    }
    if (type == self->atom_long_uri && atom->size >= sizeof(int64_t)) {
        *real = ((const LV2_Atom_Long *)atom)->body;
        return true;
    }
    if (type == self->atom_float_uri && atom->size >= sizeof(float)) {
        *real = ((const LV2_Atom_Float *)atom)->body;
        return true;
    }
    if (type == self->atom_double_uri && atom->size >= sizeof(double)) {
        *real = ((const LV2_Atom_Double *)atom)->body;
        return true;
    }

    return false;
}

static bool
sfizz_atom_extract_integer(sfizz_plugin_t *self, const LV2_Atom *atom, int64_t *integer)
{
    if (!atom)
        return false;

    const LV2_URID type = atom->type;

    if (type == self->atom_int_uri && atom->size >= sizeof(int32_t)) {
        *integer = ((const LV2_Atom_Int *)atom)->body;
        return true;
    }
    if (type == self->atom_long_uri && atom->size >= sizeof(int64_t)) {
        *integer = ((const LV2_Atom_Long *)atom)->body;
        return true;
    }
    if (type == self->atom_float_uri && atom->size >= sizeof(float)) {
        *integer = (int64_t)((const LV2_Atom_Float *)atom)->body;
        return true;
    }
    if (type == self->atom_double_uri && atom->size >= sizeof(double)) {
        *integer = (int64_t)((const LV2_Atom_Double *)atom)->body;
        return true;
    }

    return false;
}

static void
connect_port(LV2_Handle instance,
             uint32_t port,
             void *data)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    switch (port)
    {
    case SFIZZ_CONTROL:
        self->control_port = (const LV2_Atom_Sequence *)data;
        break;
    case SFIZZ_NOTIFY:
        self->notify_port = (LV2_Atom_Sequence *)data;
        break;
    case SFIZZ_AUTOMATE:
        self->automate_port = (LV2_Atom_Sequence *)data;
        break;
    case SFIZZ_LEFT:
        self->output_buffers[0] = (float *)data;
        break;
    case SFIZZ_RIGHT:
        self->output_buffers[1] = (float *)data;
        break;
    case SFIZZ_VOLUME:
        self->volume_port = (const float *)data;
        break;
    case SFIZZ_POLYPHONY:
        self->polyphony_port = (const float *)data;
        break;
    case SFIZZ_OVERSAMPLING:
        self->oversampling_port = (const float *)data;
        break;
    case SFIZZ_PRELOAD:
        self->preload_port = (const float *)data;
        break;
    case SFIZZ_FREEWHEELING:
        self->freewheel_port = (const float *)data;
        break;
    case SFIZZ_SCALA_ROOT_KEY:
        self->scala_root_key_port = (const float *)data;
        break;
    case SFIZZ_TUNING_FREQUENCY:
        self->tuning_frequency_port = (const float *)data;
        break;
    case SFIZZ_STRETCH_TUNING:
        self->stretch_tuning_port = (const float *)data;
        break;
    case SFIZZ_SAMPLE_QUALITY:
        self->sample_quality_port = (const float *)data;
        break;
    case SFIZZ_OSCILLATOR_QUALITY:
        self->oscillator_quality_port = (const float *)data;
        break;
    case SFIZZ_ACTIVE_VOICES:
        self->active_voices_port = (float *)data;
        break;
    case SFIZZ_NUM_CURVES:
        self->num_curves_port = (float *)data;
        break;
    case SFIZZ_NUM_MASTERS:
        self->num_masters_port = (float *)data;
        break;
    case SFIZZ_NUM_GROUPS:
        self->num_groups_port = (float *)data;
        break;
    case SFIZZ_NUM_REGIONS:
        self->num_regions_port = (float *)data;
        break;
    case SFIZZ_NUM_SAMPLES:
        self->num_samples_port = (float *)data;
        break;
    default:
        break;
    }
}

// This function sets the sample rate in the self parameter but does not update it.
static void
sfizz_lv2_parse_sample_rate(sfizz_plugin_t* self, const LV2_Options_Option* opt)
{
    if (opt->type == self->atom_float_uri)
    {
        // self->sample_rate = *(float*)opt->value;
        LV2_DEBUG("Attempted to change the sample rate to %.2f (original was %.2f); ignored",
                  *(float*)opt->value,
                  self->sample_rate);
    }
    else if (opt->type == self->atom_int_uri)
    {
        // self->sample_rate = *(int*)opt->value;
        LV2_DEBUG("Attempted to change the sample rate to %d (original was %.2f); ignored",
                  *(int*)opt->value,
                  self->sample_rate);
    }
    else
    {
        lv2_log_warning(&self->logger, "[sfizz] Got a sample rate but could not resolve the type of the atom\n");
        if (self->unmap)
            lv2_log_warning(&self->logger,
                            "[sfizz] Atom URI: %s\n",
                            self->unmap->unmap(self->unmap->handle, opt->type));
    }
}

static void
sfizz_lv2_get_default_sfz_path(sfizz_plugin_t *self, char *path, size_t size)
{
    snprintf(path, size, "%s/%s", self->bundle_path, DEFAULT_SFZ_FILE);
}

static void
sfizz_lv2_get_default_scala_path(sfizz_plugin_t *self, char *path, size_t size)
{
    snprintf(path, size, "%s/%s", self->bundle_path, DEFAULT_SCALA_FILE);
}

static void
sfizz_lv2_update_timeinfo(sfizz_plugin_t *self, int delay, int updates)
{
    if (updates & SFIZZ_TIMEINFO_POSITION)
        sfizz_send_time_position(self->synth, delay, self->bar, self->bar_beat);
    if (updates & SFIZZ_TIMEINFO_SIGNATURE)
        sfizz_send_time_signature(self->synth, delay, self->beats_per_bar, self->beat_unit);
    if (updates & SFIZZ_TIMEINFO_TEMPO)
        sfizz_send_bpm_tempo(self->synth, delay, self->bpm_tempo);
    if (updates & SFIZZ_TIMEINFO_SPEED)
        sfizz_send_playback_state(self->synth, delay, self->speed > 0);
}

static void
sfizz_lv2_receive_message(void* data, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    (void)delay;

    sfizz_plugin_t *self = (sfizz_plugin_t *)data;

    // transmit to UI as OSC blob
    uint8_t *osc_temp = self->osc_temp;
    uint32_t osc_size = sfizz_prepare_message(osc_temp, OSC_TEMP_SIZE, path, sig, args);
    if (osc_size > OSC_TEMP_SIZE)
        return;

    LV2_Atom_Forge* forge = &self->forge_notify;
    bool write_ok =
        lv2_atom_forge_frame_time(forge, 0) &&
        lv2_atom_forge_atom(forge, osc_size, self->sfizz_osc_blob_uri) &&
        lv2_atom_forge_raw(forge, osc_temp, osc_size);
    if (write_ok)
        lv2_atom_forge_pad(forge, osc_size);

    (void)write_ok;
}

static LV2_Handle
instantiate(const LV2_Descriptor *descriptor,
            double rate,
            const char *bundle_path,
            const LV2_Feature *const *features)
{
    UNUSED(descriptor);
    LV2_Options_Option *options = NULL;
    bool supports_bounded_block_size = false;
    bool options_has_block_size = false;
    bool supports_fixed_block_size = false;

    // Allocate and initialise instance structure.
    sfizz_plugin_t *self = new sfizz_plugin_t;
    if (!self)
        return NULL;

    strncpy(self->bundle_path, bundle_path, MAX_BUNDLE_PATH_SIZE);
    self->bundle_path[MAX_BUNDLE_PATH_SIZE - 1] = '\0';

    // Set defaults
    self->max_block_size = MAX_BLOCK_SIZE;
    self->sample_rate = (float)rate;
    self->expect_nominal_block_length = false;
    self->sfz_file_path[0] = '\0';
    self->scala_file_path[0] = '\0';
    self->num_voices = DEFAULT_VOICES;
    self->oversampling = DEFAULT_OVERSAMPLING;
    self->preload_size = DEFAULT_PRELOAD;
    self->stretch_tuning = 0.0f;
    self->check_modification = false;
    self->sample_counter = 0;

    // Initial timing
    self->bar = 0;
    self->bar_beat = 0;
    self->beats_per_bar = 4;
    self->beat_unit = 4;
    self->bpm_tempo = 120;
    self->speed = 1;

    // Get the features from the host and populate the structure
    for (const LV2_Feature *const *f = features; *f; f++)
    {
        const char *uri = (**f).URI;
        void *data = (**f).data;

        // lv2_log_note(&self->logger, "Feature URI: %s\n", (**f).URI);

        if (!strcmp(uri, LV2_URID__map))
            self->map = (LV2_URID_Map *)data;

        if (!strcmp(uri, LV2_URID__unmap))
            self->unmap = (LV2_URID_Unmap *)data;

        if (!strcmp(uri, LV2_BUF_SIZE__boundedBlockLength))
            supports_bounded_block_size = true;

        if (!strcmp(uri, LV2_BUF_SIZE__fixedBlockLength))
            supports_fixed_block_size = true;

        if (!strcmp(uri, LV2_OPTIONS__options))
            options = (LV2_Options_Option *)data;

        if (!strcmp(uri, LV2_WORKER__schedule))
            self->worker = (LV2_Worker_Schedule *)data;

        if (!strcmp(uri, LV2_LOG__log))
            self->log = (LV2_Log_Log *)data;

        if (!strcmp(uri, LV2_MIDNAM__update))
            self->midnam = (LV2_Midnam *)data;
    }

    // Setup the loggers
    lv2_log_logger_init(&self->logger, self->map, self->log);

    // The map feature is required
    if (!self->map)
    {
        lv2_log_error(&self->logger, "Map feature not found, aborting..\n");
        delete self;
        return NULL;
    }

    // The worker feature is required
    if (!self->worker)
    {
        lv2_log_error(&self->logger, "Worker feature not found, aborting..\n");
        delete self;
        return NULL;
    }

    // Map the URIs we will need
    sfizz_lv2_map_required_uris(self);

    // Initialize the forge
    lv2_atom_forge_init(&self->forge_notify, self->map);
    lv2_atom_forge_init(&self->forge_automate, self->map);
    lv2_atom_forge_init(&self->forge_secondary, self->map);

    // Check the options for the block size and sample rate parameters
    if (options)
    {
        for (const LV2_Options_Option *opt = options; opt->key || opt->value; ++opt)
        {
            if (opt->key == self->sample_rate_uri)
            {
                sfizz_lv2_parse_sample_rate(self, opt);
            }
            else if (!self->expect_nominal_block_length && opt->key == self->max_block_length_uri)
            {
                if (opt->type != self->atom_int_uri)
                {
                    lv2_log_warning(&self->logger, "Got a max block size but the type was wrong\n");
                    continue;
                }
                self->max_block_size = *(int *)opt->value;
                options_has_block_size = true;
            }
            else if (opt->key == self->nominal_block_length_uri)
            {
                if (opt->type != self->atom_int_uri)
                {
                    lv2_log_warning(&self->logger, "Got a nominal block size but the type was wrong\n");
                    continue;
                }
                self->max_block_size = *(int *)opt->value;
                self->expect_nominal_block_length = true;
                options_has_block_size = true;
            }
        }
    }
    else
    {
        lv2_log_warning(&self->logger,
                        "No option array was given upon instantiation; will use default values\n.");
    }

    // We need _some_ information on the block size
    if (!supports_bounded_block_size && !supports_fixed_block_size && !options_has_block_size)
    {
        lv2_log_error(&self->logger,
                      "Bounded block size not supported and options gave no block size, aborting..\n");
        delete self;
        return NULL;
    }

    self->ccmap = sfizz_lv2_ccmap_create(self->map);
    self->cc_current = new float[sfz::config::numCCs]();
    self->ccauto = new absl::optional<float>[sfz::config::numCCs];

    self->synth = sfizz_create_synth();
    self->client = sfizz_create_client(self);
    self->synth_mutex = spin_mutex_create();
    sfizz_set_broadcast_callback(self->synth, &sfizz_lv2_receive_message, self);
    sfizz_set_receive_callback(self->client, &sfizz_lv2_receive_message);

    self->sfz_blob_mutex = new std::mutex;

    sfizz_lv2_load_file(self, self->sfz_file_path);
    sfizz_lv2_load_scala_file(self, self->scala_file_path);

    sfizz_lv2_update_timeinfo(self, 0, ~0);

    return (LV2_Handle)self;
}

static void
cleanup(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    delete[] self->sfz_blob_data;
    delete self->sfz_blob_mutex;
    spin_mutex_destroy(self->synth_mutex);
    sfizz_delete_client(self->client);
    sfizz_free(self->synth);
    delete[] self->ccauto;
    delete[] self->cc_current;
    sfizz_lv2_ccmap_free(self->ccmap);
    delete self;
}

static void
activate(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    sfizz_set_samples_per_block(self->synth, self->max_block_size);
    sfizz_set_sample_rate(self->synth, self->sample_rate);
    self->must_update_midnam.store(1);
}

static void
deactivate(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    sfizz_all_sound_off(self->synth);
}

static void
sfizz_lv2_send_file_path(sfizz_plugin_t *self, LV2_Atom_Forge* forge, LV2_URID urid, const char *path)
{
    LV2_Atom_Forge_Frame frame;

    bool write_ok =
        lv2_atom_forge_frame_time(forge, 0) &&
        lv2_atom_forge_object(forge, &frame, 0, self->patch_set_uri) &&
        lv2_atom_forge_key(forge, self->patch_property_uri) &&
        lv2_atom_forge_urid(forge, urid) &&
        lv2_atom_forge_key(forge, self->patch_value_uri) &&
        lv2_atom_forge_path(forge, path, (uint32_t)strlen(path));

    if (write_ok)
        lv2_atom_forge_pop(forge, &frame);
}

static void
sfizz_lv2_send_controller(sfizz_plugin_t *self, LV2_Atom_Forge* forge, unsigned cc, float value)
{
    LV2_URID urid = sfizz_lv2_ccmap_map(self->ccmap, int(cc));
    LV2_Atom_Forge_Frame frame;

    bool write_ok =
        lv2_atom_forge_frame_time(forge, 0) &&
        lv2_atom_forge_object(forge, &frame, 0, self->patch_set_uri) &&
        lv2_atom_forge_key(forge, self->patch_property_uri) &&
        lv2_atom_forge_urid(forge, urid) &&
        lv2_atom_forge_key(forge, self->patch_value_uri) &&
        lv2_atom_forge_float(forge, value);

    if (write_ok)
        lv2_atom_forge_pop(forge, &frame);
}

static void
sfizz_lv2_handle_atom_object(sfizz_plugin_t *self, int delay, const LV2_Atom_Object *obj)
{
    const LV2_Atom *property = NULL;
    lv2_atom_object_get(obj, self->patch_property_uri, &property, 0);
    if (!property)
    {
        lv2_log_error(&self->logger,
                      "[sfizz] Could not get the property from the patch object, aborting\n");
        return;
    }

    if (property->type != self->atom_urid_uri)
    {
        lv2_log_error(&self->logger,
                      "[sfizz] Atom type was not a URID, aborting\n");
        return;
    }

    const uint32_t key = ((const LV2_Atom_URID *)property)->body;
    const LV2_Atom *atom = NULL;
    lv2_atom_object_get(obj, self->patch_value_uri, &atom, 0);
    if (!atom)
    {
        lv2_log_error(&self->logger, "[sfizz] Error retrieving the atom, aborting\n");
        if (self->unmap)
            lv2_log_warning(&self->logger,
                            "Atom URI: %s\n",
                            self->unmap->unmap(self->unmap->handle, key));
        return;
    }

    typedef struct
    {
        LV2_Atom atom;
        char body[MAX_PATH_SIZE];
    } sfizz_path_atom_buffer_t;

    int cc = sfizz_lv2_ccmap_unmap(self->ccmap, key);
    if (cc != -1) {
        if (atom->type == self->atom_float_uri && atom->size == sizeof(float)) {
            float value = *(const float *)LV2_ATOM_BODY_CONST(atom);
            sfizz_send_hdcc(self->synth, delay, cc, value);
            self->cc_current[cc] = value;
            self->ccauto[cc] = absl::nullopt;
        }
    }
    else if (key == self->sfizz_sfz_file_uri)
    {
        LV2_Atom_Forge *forge = &self->forge_secondary;
        sfizz_path_atom_buffer_t buffer;
        lv2_atom_forge_set_buffer(forge, (uint8_t *)&buffer, sizeof(buffer));
        const char *body = (const char *)LV2_ATOM_BODY_CONST(atom);
        uint32_t size = (uint32_t)strnlen(body, atom->size);
        if (lv2_atom_forge_typed_string(forge, self->sfizz_sfz_file_uri, body, size))
            self->worker->schedule_work(self->worker->handle, lv2_atom_total_size(&buffer.atom), &buffer.atom);
        self->check_modification = false;
    }
    else if (key == self->sfizz_scala_file_uri)
    {
        LV2_Atom_Forge *forge = &self->forge_secondary;
        sfizz_path_atom_buffer_t buffer;
        lv2_atom_forge_set_buffer(forge, (uint8_t *)&buffer, sizeof(buffer));
        const char *body = (const char *)LV2_ATOM_BODY_CONST(atom);
        uint32_t size = (uint32_t)strnlen(body, atom->size);
        if (lv2_atom_forge_typed_string(forge, self->sfizz_scala_file_uri, body, size))
            self->worker->schedule_work(self->worker->handle, lv2_atom_total_size(&buffer.atom), &buffer.atom);
        self->check_modification = false;
    }
    else
    {
        lv2_log_warning(&self->logger, "[sfizz] Unknown or unsupported object\n");
        if (self->unmap)
            lv2_log_warning(&self->logger,
                            "Object URI: %s\n",
                            self->unmap->unmap(self->unmap->handle, key));
        return;
    }
}

static void
sfizz_lv2_process_midi_event(sfizz_plugin_t *self, const LV2_Atom_Event *ev)
{
    const uint8_t *const msg = (const uint8_t *)(ev + 1);
    switch (lv2_midi_message_type(msg))
    {
    case LV2_MIDI_MSG_NOTE_ON:
        if (msg[2] == 0)
            goto noteoff; // 0 velocity note-ons should be forbidden but just in case...

        sfizz_send_note_on(self->synth,
                           (int)ev->time.frames,
                           (int)msg[1],
                           msg[2]);
        break;
    case LV2_MIDI_MSG_NOTE_OFF: noteoff:
        sfizz_send_note_off(self->synth,
                            (int)ev->time.frames,
                            (int)msg[1],
                            msg[2]);
        break;
    // Note(jpc) CC must be mapped by host, not handled here.
    //           See LV2 midi:binding.
#if defined(SFIZZ_LV2_PSA)
    case LV2_MIDI_MSG_CONTROLLER:
        {
            unsigned cc = msg[1];
            float value = float(msg[2]) * (1.0f / 127.0f);
            sfizz_send_hdcc(self->synth,
                            (int)ev->time.frames,
                            (int)cc,
                            value);
            self->cc_current[cc] = value;
            self->ccauto[cc] = value;
            self->have_ccauto = true;
        }
        break;
#endif
    case LV2_MIDI_MSG_CHANNEL_PRESSURE:
        sfizz_send_channel_aftertouch(self->synth,
                      (int)ev->time.frames,
                      msg[1]);
        break;
    case LV2_MIDI_MSG_NOTE_PRESSURE:
        sfizz_send_poly_aftertouch(self->synth,
                      (int)ev->time.frames,
                      (int)msg[1],
                      msg[2]);
        break;
    case LV2_MIDI_MSG_BENDER:
        sfizz_send_pitch_wheel(self->synth,
                        (int)ev->time.frames,
                        PITCH_BUILD_AND_CENTER(msg[1], msg[2]));
        break;
    default:
        break;
    }
}

static void
sfizz_lv2_status_log(sfizz_plugin_t *self)
{
    UNUSED(self);
    // lv2_log_note(&self->logger, "[sfizz] Allocated buffers: %d\n", sfizz_get_num_buffers(self->synth));
    // lv2_log_note(&self->logger, "[sfizz] Allocated bytes: %d bytes\n", sfizz_get_num_bytes(self->synth));
    // lv2_log_note(&self->logger, "[sfizz] Active voices: %d\n", sfizz_get_num_active_voices(self->synth));
}

static int next_pow_2(int v)
{
    if (v < 1)
        return 1;

    // Bit twiddling hack
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static void
sfizz_lv2_check_oversampling(sfizz_plugin_t* self)
{
    int port_value = next_pow_2((int)*self->oversampling_port);
    if (port_value == (int)self->oversampling)
        return;

    self->oversampling = (sfizz_oversampling_factor_t)port_value;

    LV2_Atom_Int atom;
    atom.atom.type = self->sfizz_oversampling_uri;
    atom.atom.size = sizeof(int);
    atom.body = self->oversampling;
    if (self->worker->schedule_work(self->worker->handle,
                                    lv2_atom_total_size((LV2_Atom *)&atom),
                                    &atom) != LV2_WORKER_SUCCESS)
    {
        lv2_log_error(&self->logger, "[sfizz] There was an issue changing the oversampling factor\n");
    }
}

static void
sfizz_lv2_check_preload_size(sfizz_plugin_t* self)
{
    unsigned int preload_size = (int)*self->preload_port;
    if (preload_size != self->preload_size)
    {
        LV2_Atom_Int atom;
        atom.atom.type = self->sfizz_preload_size_uri;
        atom.atom.size = sizeof(int);
        atom.body = preload_size;
        if (self->worker->schedule_work(self->worker->handle,
                                        lv2_atom_total_size((LV2_Atom *)&atom),
                                        &atom) != LV2_WORKER_SUCCESS)
        {
            lv2_log_error(&self->logger, "[sfizz] There was an issue changing the preload size\n");
        }
        self->preload_size = preload_size;
    }
}

static void
sfizz_lv2_check_num_voices(sfizz_plugin_t* self)
{
    int num_voices = (int)*self->polyphony_port;
    if (num_voices != self->num_voices)
    {
        LV2_Atom_Int atom;
        atom.atom.type = self->sfizz_num_voices_uri;
        atom.atom.size = sizeof(int);
        atom.body = num_voices;
        if (self->worker->schedule_work(self->worker->handle,
                                        lv2_atom_total_size((LV2_Atom *)&atom),
                                        &atom) != LV2_WORKER_SUCCESS)
        {
            lv2_log_error(&self->logger, "[sfizz] There was an issue changing the number of voices\n");
        }
        self->num_voices = num_voices;
    }
}

static void
sfizz_lv2_check_freewheeling(sfizz_plugin_t* self)
{
    if (*(self->freewheel_port) > 0)
    {
        sfizz_enable_freewheeling(self->synth);
    }
    else
    {
        sfizz_disable_freewheeling(self->synth);
    }
}

static void
sfizz_lv2_check_stretch_tuning(sfizz_plugin_t* self)
{
    float stretch_tuning = (float)*self->stretch_tuning_port;
    if (stretch_tuning != self->stretch_tuning)
    {
        sfizz_load_stretch_tuning_by_ratio(self->synth, stretch_tuning);
        self->stretch_tuning = stretch_tuning;
    }
}

static void
run(LV2_Handle instance, uint32_t sample_count)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    assert(self->control_port && self->notify_port && self->automate_port);

    if (!spin_mutex_trylock(self->synth_mutex))
    {
        for (int channel = 0; channel < 2; ++channel)
            memset(self->output_buffers[channel], 0, sample_count * sizeof(float));
        return;
    }

    // Set up dedicated forges to write on their respective ports.
    const size_t notify_capacity = self->notify_port->atom.size;
    lv2_atom_forge_set_buffer(&self->forge_notify, (uint8_t *)self->notify_port, notify_capacity);
    const size_t automate_capacity = self->automate_port->atom.size;
    lv2_atom_forge_set_buffer(&self->forge_automate, (uint8_t *)self->automate_port, automate_capacity);

    // Start sequences in the respective output ports.
    LV2_Atom_Forge_Frame notify_frame;
    if (!lv2_atom_forge_sequence_head(&self->forge_notify, &notify_frame, 0))
        assert(false);
    LV2_Atom_Forge_Frame automate_frame;
    if (!lv2_atom_forge_sequence_head(&self->forge_automate, &automate_frame, 0))
        assert(false);

    LV2_ATOM_SEQUENCE_FOREACH(self->control_port, ev)
    {
        const int delay = (int)ev->time.frames;

        // If the received atom is an object/patch message
        if (ev->body.type == self->atom_object_uri || ev->body.type == self->atom_blank_uri)
        {
            const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;

            if (obj->body.otype == self->patch_set_uri)
            {
                sfizz_lv2_handle_atom_object(self, delay, obj);
            }
            else if (obj->body.otype == self->patch_get_uri)
            {
                const LV2_Atom_URID *property = NULL;
                lv2_atom_object_get(obj, self->patch_property_uri, &property, 0);
                if (!property) // Send the full state
                {
                    sfizz_lv2_send_file_path(self, &self->forge_notify, self->sfizz_sfz_file_uri, self->sfz_file_path);
                    sfizz_lv2_send_file_path(self, &self->forge_notify, self->sfizz_scala_file_uri, self->scala_file_path);

                    for (unsigned cc = 0; cc < sfz::config::numCCs; ++cc)
                        sfizz_lv2_send_controller(self, &self->forge_notify, cc, self->cc_current[cc]);
                }
                else if (property->body == self->sfizz_sfz_file_uri)
                {
                    sfizz_lv2_send_file_path(self, &self->forge_notify, self->sfizz_sfz_file_uri, self->sfz_file_path);
                }
                else if (property->body == self->sfizz_scala_file_uri)
                {
                    sfizz_lv2_send_file_path(self, &self->forge_notify, self->sfizz_scala_file_uri, self->scala_file_path);
                }
                else
                {
                    int cc = sfizz_lv2_ccmap_unmap(self->ccmap, property->body);
                    if (cc != -1)
                        sfizz_lv2_send_controller(self, &self->forge_notify, unsigned(cc), self->cc_current[cc]);
                }
            }
            else if (obj->body.otype == self->time_position_uri)
            {
                const LV2_Atom *bar_atom = NULL;
                const LV2_Atom *bar_beat_atom = NULL;
                const LV2_Atom *beat_unit_atom = NULL;
                const LV2_Atom *beats_per_bar_atom = NULL;
                const LV2_Atom *beats_per_minute_atom = NULL;
                const LV2_Atom *speed_atom = NULL;

                lv2_atom_object_get(
                    obj,
                    self->time_bar_uri, &bar_atom,
                    self->time_bar_beat_uri, &bar_beat_atom,
                    self->time_beats_per_bar_uri, &beats_per_bar_atom,
                    self->time_beats_per_minute_uri, &beats_per_minute_atom,
                    self->time_beat_unit_uri, &beat_unit_atom,
                    self->time_speed_uri, &speed_atom,
                    0);

                int updates = 0;

                int64_t bar;
                double bar_beat;
                if (sfizz_atom_extract_integer(self, bar_atom, &bar)) {
                    self->bar = (int)bar;
                    updates |= SFIZZ_TIMEINFO_POSITION;
                }
                if (sfizz_atom_extract_real(self, bar_beat_atom, &bar_beat)) {
                    self->bar_beat = (float)bar_beat;
                    updates |= SFIZZ_TIMEINFO_POSITION;
                }

                double beats_per_bar;
                int64_t beat_unit;
                if (sfizz_atom_extract_real(self, beats_per_bar_atom, &beats_per_bar)) {
                    self->beats_per_bar = (int)beats_per_bar;
                    updates |= SFIZZ_TIMEINFO_SIGNATURE;
                }
                if (sfizz_atom_extract_integer(self, beat_unit_atom, &beat_unit)) {
                    self->beat_unit = (int)beat_unit;
                    updates |= SFIZZ_TIMEINFO_SIGNATURE;
                }

                double tempo;
                if (sfizz_atom_extract_real(self, beats_per_minute_atom, &tempo)) {
                    self->bpm_tempo = (float)tempo;
                    updates |= SFIZZ_TIMEINFO_TEMPO;
                }

                double speed;
                if (sfizz_atom_extract_real(self, speed_atom, &speed)) {
                    self->speed = (float)speed;
                    updates |= SFIZZ_TIMEINFO_SPEED;
                }

                sfizz_lv2_update_timeinfo(self, delay, updates);
            }
            else
            {
                lv2_log_warning(&self->logger, "[sfizz] Got an Object atom but it was not supported\n");
                if (self->unmap)
                    lv2_log_warning(&self->logger,
                                    "Object URI: %s\n",
                                    self->unmap->unmap(self->unmap->handle, obj->body.otype));
                continue;
            }
        }
        else if (ev->body.type == self->midi_event_uri)
        {
            // Got an atom that is a MIDI event
            sfizz_lv2_process_midi_event(self, ev);
        }
        else if (ev->body.type == self->sfizz_osc_blob_uri)
        {
            // Got an atom that is a OSC event
            const char *path;
            const char *sig;
            const sfizz_arg_t *args;
            uint8_t buffer[1024];
            if (sfizz_extract_message(LV2_ATOM_BODY_CONST(&ev->body), ev->body.size, buffer, sizeof(buffer), &path, &sig, &args) > 0)
                sfizz_send_message(self->synth, self->client, (int)ev->time.frames, path, sig, args);
        }
    }


    // Check and update parameters if needed
    sfizz_lv2_check_freewheeling(self);
    sfizz_set_volume(self->synth, *(self->volume_port));
    sfizz_set_scala_root_key(self->synth, *(self->scala_root_key_port));
    sfizz_set_tuning_frequency(self->synth, *(self->tuning_frequency_port));
    sfizz_set_sample_quality(self->synth, SFIZZ_PROCESS_LIVE, (int)(*self->sample_quality_port));
    sfizz_set_oscillator_quality(self->synth, SFIZZ_PROCESS_LIVE, (int)(*self->oscillator_quality_port));
    sfizz_lv2_check_stretch_tuning(self);
    sfizz_lv2_check_preload_size(self);
    sfizz_lv2_check_oversampling(self);
    sfizz_lv2_check_num_voices(self);
    *(self->active_voices_port) = sfizz_get_num_active_voices(self->synth);
    *(self->num_curves_port) = sfizz_get_num_curves(self->synth);
    *(self->num_masters_port) = sfizz_get_num_masters(self->synth);
    *(self->num_groups_port) = sfizz_get_num_groups(self->synth);
    *(self->num_regions_port) = sfizz_get_num_regions(self->synth);
    *(self->num_samples_port) = sfizz_get_num_preloaded_samples(self->synth);

    // Log the buffer usage
    self->sample_counter += (int)sample_count;
    if (self->sample_counter > LOG_SAMPLE_COUNT && self->check_modification)
    {
        LV2_Atom atom;
        atom.size = 0;
#ifndef NDEBUG
        atom.type = self->sfizz_log_status_uri;
        if (!(self->worker->schedule_work(self->worker->handle,
                                         lv2_atom_total_size((LV2_Atom *)&atom),
                                         &atom) == LV2_WORKER_SUCCESS))
        {
            lv2_log_error(&self->logger, "[sfizz] There was an issue sending a logging message to the background worker\n");
        }
#endif
        atom.type = self->sfizz_check_modification_uri;
        if ((self->worker->schedule_work(self->worker->handle,
                                        lv2_atom_total_size((LV2_Atom *)&atom),
                                        &atom) == LV2_WORKER_SUCCESS)) {
            self->check_modification = false;
        } else {
            lv2_log_error(&self->logger, "[sfizz] There was an issue sending a notice to check the modification of the SFZ file to the background worker\n");
        }
        self->sample_counter = 0;
    }

    // Render the block
    sfizz_render_block(self->synth, self->output_buffers, 2, (int)sample_count);

    // Request OSC updates
    sfizz_send_message(self->synth, self->client, 0, "/sw/last/current", "", nullptr);

    if (self->midnam && self->must_update_midnam.exchange(0))
    {
        self->midnam->update(self->midnam->handle);
    }

    if (self->have_ccauto)
    {
        for (unsigned cc = 0; cc < sfz::config::numCCs; ++cc) {
            absl::optional<float> value = self->ccauto[cc];
            if (value) {
                sfizz_lv2_send_controller(self, &self->forge_automate, cc, *value);
                self->ccauto[cc] = absl::nullopt;
            }
        }
        self->have_ccauto = false;
    }

    spin_mutex_unlock(self->synth_mutex);

    lv2_atom_forge_pop(&self->forge_notify, &notify_frame);
    lv2_atom_forge_pop(&self->forge_automate, &automate_frame);
}

static uint32_t
lv2_get_options(LV2_Handle instance, LV2_Options_Option *options)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    LV2_DEBUG("[DEBUG] get_options called\n");
    for (LV2_Options_Option *opt = options; opt->key || opt->value; ++opt)
    {
        if (self->unmap) {
            LV2_DEBUG("[DEBUG] Called for an option with key (subject): %s (%s) \n",
                      self->unmap->unmap(self->unmap->handle, opt->key),
                      self->unmap->unmap(self->unmap->handle, opt->subject));
        }

        if (opt->key == self->sample_rate_uri)
        {
            opt->type = self->atom_float_uri;
            opt->size = sizeof(float);
            opt->value = (void*)&self->sample_rate;
            return LV2_OPTIONS_SUCCESS;
        }

        if (opt->key == self->max_block_length_uri || opt->key == self->nominal_block_length_uri)
        {
            opt->type = self->atom_int_uri;
            opt->size = sizeof(int);
            opt->value = (void*)&self->max_block_size;
            return LV2_OPTIONS_SUCCESS;
        }
    }
    return LV2_OPTIONS_ERR_UNKNOWN;
}

static uint32_t
lv2_set_options(LV2_Handle instance, const LV2_Options_Option *options)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    // Update the block size and sample rate as needed
    for (const LV2_Options_Option *opt = options; opt->key || opt->value; ++opt)
    {
        if (opt->key == self->sample_rate_uri)
        {
            sfizz_lv2_parse_sample_rate(self, opt);
            spin_mutex_lock(self->synth_mutex);
            sfizz_set_sample_rate(self->synth, self->sample_rate);
            spin_mutex_unlock(self->synth_mutex);
        }
        else if (!self->expect_nominal_block_length && opt->key == self->max_block_length_uri)
        {
            if (opt->type != self->atom_int_uri)
            {
                lv2_log_warning(&self->logger, "[sfizz] Got a max block size but the type was wrong\n");
                continue;
            }
            self->max_block_size = *(int *)opt->value;
            spin_mutex_lock(self->synth_mutex);
            sfizz_set_samples_per_block(self->synth, self->max_block_size);
            spin_mutex_unlock(self->synth_mutex);
        }
        else if (opt->key == self->nominal_block_length_uri)
        {
            if (opt->type != self->atom_int_uri)
            {
                lv2_log_warning(&self->logger, "[sfizz] Got a nominal block size but the type was wrong\n");
                continue;
            }
            self->max_block_size = *(int *)opt->value;
            spin_mutex_lock(self->synth_mutex);
            sfizz_set_samples_per_block(self->synth, self->max_block_size);
            spin_mutex_unlock(self->synth_mutex);
        }
    }
    return LV2_OPTIONS_SUCCESS;
}

static void
sfizz_lv2_update_file_info(sfizz_plugin_t* self, const char *file_path)
{
    if (file_path != self->sfz_file_path)
        strcpy(self->sfz_file_path, file_path);

    lv2_log_note(&self->logger, "[sfizz] File changed to: %s\n", file_path);

    char *unknown_opcodes = sfizz_get_unknown_opcodes(self->synth);
    if (unknown_opcodes)
    {
        lv2_log_note(&self->logger, "[sfizz] Unknown opcodes: %s\n", unknown_opcodes);
        free(unknown_opcodes);
    }
    lv2_log_note(&self->logger, "[sfizz] Number of masters: %d\n", sfizz_get_num_masters(self->synth));
    lv2_log_note(&self->logger, "[sfizz] Number of groups: %d\n", sfizz_get_num_groups(self->synth));
    lv2_log_note(&self->logger, "[sfizz] Number of regions: %d\n", sfizz_get_num_regions(self->synth));

    self->must_update_midnam.store(1);
}

static void
sfizz_lv2_update_sfz_info(sfizz_plugin_t *self)
{
    const std::string blob = getDescriptionBlob(self->synth);

    // Update description blob that UI can fetch, thread-safely
    uint32_t size = uint32_t(blob.size());
    uint8_t *data = new uint8_t[size];
    memcpy(data, blob.data(), size);

    self->sfz_blob_mutex->lock();
    self->sfz_blob_serial += 1;
    const uint8_t *old_data = self->sfz_blob_data;
    self->sfz_blob_data = data;
    self->sfz_blob_size = size;
    self->sfz_blob_mutex->unlock();

    delete[] old_data;

    //
    const InstrumentDescription desc = parseDescriptionBlob(blob);
    for (unsigned cc = 0; cc < sfz::config::numCCs; ++cc) {
        if (desc.ccUsed.test(cc)) {
            // Mark all the used CCs for automation with default values
            self->ccauto[cc] = desc.ccDefault[cc];
            self->have_ccauto = true;
            // Update the current CCs
            self->cc_current[cc] = desc.ccDefault[cc];
        }
    }
}

static bool
sfizz_lv2_load_file(sfizz_plugin_t *self, const char *file_path)
{
    bool status;

    char buf[MAX_PATH_SIZE];
    if (file_path[0] == '\0')
    {
        sfizz_lv2_get_default_sfz_path(self, buf, MAX_PATH_SIZE);
        file_path = buf;
    }

    ///
    const sfz::InstrumentFormatRegistry& formatRegistry = sfz::InstrumentFormatRegistry::getInstance();
    const sfz::InstrumentFormat* format = formatRegistry.getMatchingFormat(file_path);

    if (!format)
        status = sfizz_load_file(self->synth, file_path);
    else {
        auto importer = format->createImporter();
        std::string virtual_path = std::string(file_path) + ".sfz";
        std::string sfz_text = importer->convertToSfz(file_path);
        status = sfizz_load_string(self->synth, virtual_path.c_str(), sfz_text.c_str());
    }

    sfizz_lv2_update_sfz_info(self);
    sfizz_lv2_update_file_info(self, file_path);
    return status;
}

static bool
sfizz_lv2_load_scala_file(sfizz_plugin_t *self, const char *file_path)
{
    char buf[MAX_PATH_SIZE];
    if (file_path[0] == '\0')
    {
        sfizz_lv2_get_default_scala_path(self, buf, MAX_PATH_SIZE);
        file_path = buf;
    }

    bool status = sfizz_load_scala_file(self->synth, file_path);
    if (file_path != self->scala_file_path)
        strcpy(self->scala_file_path, file_path);
    return status;
}

static LV2_State_Status
restore(LV2_Handle instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle handle,
        uint32_t flags,
        const LV2_Feature *const *features)
{
    UNUSED(flags);
    LV2_State_Status status = LV2_STATE_SUCCESS;
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    LV2_State_Map_Path *map_path = NULL;
    LV2_State_Free_Path *free_path = &sfizz_State_Free_Path;
    for (const LV2_Feature *const *f = features; *f; ++f)
    {
        if (!strcmp((*f)->URI, LV2_STATE__mapPath))
            map_path = (LV2_State_Map_Path *)(**f).data;
        else if (!strcmp((*f)->URI, LV2_STATE__freePath))
            free_path = (LV2_State_Free_Path *)(**f).data;
    }

    // Set default values
    sfizz_lv2_get_default_sfz_path(self, self->sfz_file_path, MAX_PATH_SIZE);
    sfizz_lv2_get_default_scala_path(self, self->scala_file_path, MAX_PATH_SIZE);
    self->num_voices = DEFAULT_VOICES;
    self->preload_size = DEFAULT_PRELOAD;
    self->oversampling = DEFAULT_OVERSAMPLING;

    // Fetch back the saved file path, if any
    size_t size;
    uint32_t type;
    uint32_t val_flags;
    const void *value;
    value = retrieve(handle, self->sfizz_sfz_file_uri, &size, &type, &val_flags);
    if (value)
    {
        const char *path = (const char *)value;
        if (map_path)
        {
            path = map_path->absolute_path(map_path->handle, path);
            if (!path)
                status = LV2_STATE_ERR_UNKNOWN;
        }

        if (path)
        {
            strncpy(self->sfz_file_path, path, MAX_PATH_SIZE);
            self->sfz_file_path[MAX_PATH_SIZE - 1] = '\0';

            if (map_path)
                free_path->free_path(free_path->handle, (char *)path);
        }
    }

    value = retrieve(handle, self->sfizz_scala_file_uri, &size, &type, &val_flags);
    if (value)
    {
        const char *path = (const char *)value;
        if (map_path)
        {
            path = map_path->absolute_path(map_path->handle, path);
            if (!path)
                status = LV2_STATE_ERR_UNKNOWN;
        }

        if (path)
        {
            strncpy(self->scala_file_path, path, MAX_PATH_SIZE);
            self->scala_file_path[MAX_PATH_SIZE - 1] = '\0';

            if (map_path)
                free_path->free_path(free_path->handle, (char *)path);
        }
    }

    value = retrieve(handle, self->sfizz_num_voices_uri, &size, &type, &val_flags);
    if (value)
    {
        int num_voices = *(const int *)value;
        if (num_voices > 0 && num_voices <= MAX_VOICES)
            self->num_voices = num_voices;
    }

    value = retrieve(handle, self->sfizz_preload_size_uri, &size, &type, &val_flags);
    if (value)
    {
        unsigned int preload_size = *(const unsigned int *)value;
        self->preload_size = preload_size;
    }

    value = retrieve(handle, self->sfizz_oversampling_uri, &size, &type, &val_flags);
    if (value)
    {
        sfizz_oversampling_factor_t oversampling = *(const sfizz_oversampling_factor_t *)value;
        self->oversampling = oversampling;
    }

    // Collect all CC values present in the state
    std::unique_ptr<absl::optional<float>[]> cc_values(
        new absl::optional<float>[sfz::config::numCCs]);

    for (unsigned cc = 0; cc < sfz::config::numCCs; ++cc) {
        LV2_URID urid = sfizz_lv2_ccmap_map(self->ccmap, int(cc));
        value = retrieve(handle, urid, &size, &type, &val_flags);
        if (value && type == self->atom_float_uri)
            cc_values[cc] = *(const float *)value;
    }

    // Sync the parameters to the synth
    spin_mutex_lock(self->synth_mutex);

    // Load an empty file to remove the default sine, and then the new file.
    sfizz_load_string(self->synth, "empty.sfz", "");
    self->check_modification = false;
    if (sfizz_lv2_load_file(self, self->sfz_file_path))
    {
        lv2_log_note(&self->logger,
            "[sfizz] Restoring the file %s\n", self->sfz_file_path);
        self->check_modification = true;
    }
    else
    {
        lv2_log_error(&self->logger,
            "[sfizz] Error while restoring the file %s\n", self->sfz_file_path);
    }

    if (sfizz_lv2_load_scala_file(self, self->scala_file_path))
    {
        lv2_log_note(&self->logger,
            "[sfizz] Restoring the scale %s\n", self->scala_file_path);
    }
    else
    {
        lv2_log_error(&self->logger,
            "[sfizz] Error while restoring the scale %s\n", self->scala_file_path);
    }

    lv2_log_note(&self->logger, "[sfizz] Restoring the number of voices to %d\n", self->num_voices);
    sfizz_set_num_voices(self->synth, self->num_voices);

    lv2_log_note(&self->logger, "[sfizz] Restoring the preload size to %d\n", self->preload_size);
    sfizz_set_preload_size(self->synth, self->preload_size);

    lv2_log_note(&self->logger, "[sfizz] Restoring the oversampling to %d\n", self->oversampling);
    sfizz_set_oversampling_factor(self->synth, self->oversampling);

    // Override default automation values with these from the state file
    for (unsigned cc = 0; cc < sfz::config::numCCs; ++cc) {
        absl::optional<float> value = cc_values[cc];
        if (value) {
            // Set CC in the synth
            sfizz_send_hdcc(self->synth, 0, int(cc), *value);
            // Mark CCs for automation with state values
            self->ccauto[cc] = *value;
            self->have_ccauto = true;
            // Update the current CCs
            self->cc_current[cc] = *value;
        }
    }

    spin_mutex_unlock(self->synth_mutex);

    return status;
}

static LV2_State_Status
save(LV2_Handle instance,
     LV2_State_Store_Function store,
     LV2_State_Handle handle,
     uint32_t flags,
     const LV2_Feature *const *features)
{
    UNUSED(flags);
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    LV2_State_Map_Path *map_path = NULL;
    LV2_State_Free_Path *free_path = &sfizz_State_Free_Path;
    for (const LV2_Feature *const *f = features; *f; ++f)
    {
        if (!strcmp((*f)->URI, LV2_STATE__mapPath))
            map_path = (LV2_State_Map_Path *)(**f).data;
        else if (!strcmp((*f)->URI, LV2_STATE__freePath))
            free_path = (LV2_State_Free_Path *)(**f).data;
    }

    const char *path;

    // Save the file path
    path = self->sfz_file_path;
    if (map_path)
    {
        path = map_path->abstract_path(map_path->handle, path);
        if (!path)
            return LV2_STATE_ERR_UNKNOWN;
    }
    store(handle,
          self->sfizz_sfz_file_uri,
          path,
          strlen(path) + 1,
          self->atom_path_uri,
          LV2_STATE_IS_POD);
    if (map_path)
        free_path->free_path(free_path->handle, (char *)path);

    // Save the scala file path
    path = self->scala_file_path;
    if (map_path)
    {
        path = map_path->abstract_path(map_path->handle, path);
        if (!path)
            return LV2_STATE_ERR_UNKNOWN;
    }
    if (!path)
        return LV2_STATE_ERR_UNKNOWN;
    store(handle,
          self->sfizz_scala_file_uri,
          path,
          strlen(path) + 1,
          self->atom_path_uri,
          LV2_STATE_IS_POD);
    if (map_path)
        free_path->free_path(free_path->handle, (char *)path);

    // Save the number of voices
    store(handle,
          self->sfizz_num_voices_uri,
          &self->num_voices,
          sizeof(int),
          self->atom_int_uri,
          LV2_STATE_IS_POD);

    // Save the preload size
    store(handle,
          self->sfizz_preload_size_uri,
          &self->preload_size,
          sizeof(unsigned int),
          self->atom_int_uri,
          LV2_STATE_IS_POD);

    // Save the preload size
    store(handle,
          self->sfizz_oversampling_uri,
          &self->oversampling,
          sizeof(int),
          self->atom_int_uri,
          LV2_STATE_IS_POD);

    // Save the CCs (used only)
    self->sfz_blob_mutex->lock();
    const InstrumentDescription desc = parseDescriptionBlob(
        absl::string_view((const char*)self->sfz_blob_data, self->sfz_blob_size));
    self->sfz_blob_mutex->unlock();

    for (unsigned cc = 0; cc < sfz::config::numCCs; ++cc) {
        if (desc.ccUsed.test(cc)) {
            LV2_URID urid = sfizz_lv2_ccmap_map(self->ccmap, int(cc));
            store(handle,
                  urid,
                  &self->cc_current[cc],
                  sizeof(float),
                  self->atom_float_uri,
                  LV2_STATE_IS_POD);
        }
    }

    return LV2_STATE_SUCCESS;
}

static void
sfizz_lv2_activate_file_checking(
    sfizz_plugin_t *self,
    LV2_Worker_Respond_Function respond,
    LV2_Worker_Respond_Handle handle)
{
    LV2_Atom check_modification_atom;
    check_modification_atom.size = 0;
    check_modification_atom.type = self->sfizz_check_modification_uri;
    respond(handle, lv2_atom_total_size(&check_modification_atom), &check_modification_atom);
}

// This runs in a lower priority thread
static LV2_Worker_Status
work(LV2_Handle instance,
     LV2_Worker_Respond_Function respond,
     LV2_Worker_Respond_Handle handle,
     uint32_t size,
     const void *data)
{
    UNUSED(size);
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    if (!data) {
        lv2_log_error(&self->logger, "[sfizz] Ignoring empty data in the worker thread\n");
        return LV2_WORKER_ERR_UNKNOWN;
    }

    const LV2_Atom *atom = (const LV2_Atom *)data;
    if (atom->type == self->sfizz_sfz_file_uri)
    {
        const char *sfz_file_path = (const char *)LV2_ATOM_BODY_CONST(atom);

        spin_mutex_lock(self->synth_mutex);
        bool success = sfizz_lv2_load_file(self, sfz_file_path);
        spin_mutex_unlock(self->synth_mutex);

        if (!success) {
            lv2_log_error(&self->logger,
                "[sfizz] Error with %s; no file should be loaded\n", sfz_file_path);
        }

        // Reactivate checking for file changes
        sfizz_lv2_activate_file_checking(self, respond, handle);
    }
    else if (atom->type == self->sfizz_scala_file_uri)
    {
        const char *scala_file_path = (const char *)LV2_ATOM_BODY_CONST(atom);

        spin_mutex_lock(self->synth_mutex);
        bool success = sfizz_lv2_load_scala_file(self, scala_file_path);
        spin_mutex_unlock(self->synth_mutex);

        if (success) {
            lv2_log_note(&self->logger, "[sfizz] Scala file loaded: %s\n", scala_file_path);
        } else {
            lv2_log_error(&self->logger,
                "[sfizz] Error with %s; no new scala file should be loaded\n", scala_file_path);
        }

        // Reactivate checking for file changes
        sfizz_lv2_activate_file_checking(self, respond, handle);
    }
    else if (atom->type == self->sfizz_num_voices_uri)
    {
        const int num_voices = *(const int *)LV2_ATOM_BODY_CONST(atom);

        spin_mutex_lock(self->synth_mutex);
        sfizz_set_num_voices(self->synth, num_voices);
        spin_mutex_unlock(self->synth_mutex);

        if (sfizz_get_num_voices(self->synth) == num_voices) {
            lv2_log_note(&self->logger, "[sfizz] Number of voices changed to: %d\n", num_voices);
        } else {
            lv2_log_error(&self->logger, "[sfizz] Error changing the number of voices\n");
        }
    }
    else if (atom->type == self->sfizz_preload_size_uri)
    {
        const unsigned int preload_size = *(const unsigned int *)LV2_ATOM_BODY_CONST(atom);

        spin_mutex_lock(self->synth_mutex);
        sfizz_set_preload_size(self->synth, preload_size);
        spin_mutex_unlock(self->synth_mutex);

        if (sfizz_get_preload_size(self->synth) == preload_size) {
            lv2_log_note(&self->logger, "[sfizz] Preload size changed to: %d\n", preload_size);
        } else {
            lv2_log_error(&self->logger, "[sfizz] Error changing the preload size\n");
        }
    }
    else if (atom->type == self->sfizz_oversampling_uri)
    {
        const sfizz_oversampling_factor_t oversampling =
            *(const sfizz_oversampling_factor_t *)LV2_ATOM_BODY_CONST(atom);

        spin_mutex_lock(self->synth_mutex);
        sfizz_set_oversampling_factor(self->synth, oversampling);
        spin_mutex_unlock(self->synth_mutex);

        if (sfizz_get_oversampling_factor(self->synth) == oversampling) {
            lv2_log_note(&self->logger, "[sfizz] Oversampling changed to: %d\n", oversampling);
        } else {
            lv2_log_error(&self->logger, "[sfizz] Error changing the oversampling\n");
        }
    }
    else if (atom->type == self->sfizz_log_status_uri)
    {
        sfizz_lv2_status_log(self);
    }
    else if (atom->type == self->sfizz_check_modification_uri)
    {
        if (sfizz_should_reload_file(self->synth))
        {
            lv2_log_note(&self->logger,
                        "[sfizz] File %s seems to have been updated, reloading\n",
                        self->sfz_file_path);

            spin_mutex_lock(self->synth_mutex);
            bool success = sfizz_lv2_load_file(self, self->sfz_file_path);
            spin_mutex_unlock(self->synth_mutex);

            if (!success) {
                lv2_log_error(&self->logger,
                    "[sfizz] Error with %s; no file should be loaded\n", self->sfz_file_path);
            }
        }

        if (sfizz_should_reload_scala(self->synth))
        {
            lv2_log_note(&self->logger,
                        "[sfizz] Scala file %s seems to have been updated, reloading\n",
                        self->scala_file_path);

            spin_mutex_lock(self->synth_mutex);
            bool success = sfizz_lv2_load_scala_file(self, self->scala_file_path);
            spin_mutex_unlock(self->synth_mutex);

            if (success) {
                lv2_log_note(&self->logger, "[sfizz] Scala file loaded: %s\n", self->scala_file_path);
            } else {
                lv2_log_error(&self->logger,
                    "[sfizz] Error with %s; no new scala file should be loaded\n", self->scala_file_path);
            }
        }

        // Reactivate checking for file changes
        sfizz_lv2_activate_file_checking(self, respond, handle);
    }
    else
    {
        lv2_log_error(&self->logger, "[sfizz] Got an unknown atom in work\n");
        if (self->unmap)
            lv2_log_error(&self->logger,
                          "URI: %s\n",
                          self->unmap->unmap(self->unmap->handle, atom->type));
        return LV2_WORKER_ERR_UNKNOWN;
    }
    return LV2_WORKER_SUCCESS;
}

// This runs in the audio thread
static LV2_Worker_Status
work_response(LV2_Handle instance,
              uint32_t size,
              const void *data)
{
    UNUSED(size);
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    if (!data)
        return LV2_WORKER_ERR_UNKNOWN;

    const LV2_Atom *atom = (const LV2_Atom *)data;
    if (atom->type == self->sfizz_check_modification_uri) {
        self->check_modification = true; // check changes
    } else {
        lv2_log_error(&self->logger, "[sfizz] Got an unexpected atom in work response\n");
        if (self->unmap)
            lv2_log_error(&self->logger,
                          "URI: %s\n",
                          self->unmap->unmap(self->unmap->handle, atom->type));
        return LV2_WORKER_ERR_UNKNOWN;
    }

    return LV2_WORKER_SUCCESS;
}

static char *
midnam_model(LV2_Handle instance)
{
    char *model = (char *)malloc(64);
    if (!model)
        return NULL;

    sprintf(model, "Sfizz LV2:%p", instance);
    return model;
}

static char *
midnam_export(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    char *model = midnam_model(instance);
    if (!model)
        return NULL;

    char *xml = sfizz_export_midnam(self->synth, model);
    free(model);
    return xml;
}

static void
midnam_free(char *string)
{
    sfizz_free_memory(string);
}

static const void *
extension_data(const char *uri)
{
    static const LV2_Options_Interface options = {lv2_get_options, lv2_set_options};
    static const LV2_State_Interface state = {save, restore};
    static const LV2_Worker_Interface worker = {work, work_response, NULL};
    static const LV2_Midnam_Interface midnam = {midnam_export, midnam_model, midnam_free};

    // Advertise the extensions we support
    if (!strcmp(uri, LV2_OPTIONS__interface))
        return &options;
    else if (!strcmp(uri, LV2_STATE__interface))
        return &state;
    else if (!strcmp(uri, LV2_WORKER__interface))
        return &worker;
    else if (!strcmp(uri, LV2_MIDNAM__interface))
        return &midnam;

    return NULL;
}

static const LV2_Descriptor descriptor = {
    SFIZZ_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    extension_data};

LV2_SYMBOL_EXPORT
const LV2_Descriptor *
lv2_descriptor(uint32_t index)
{
    switch (index)
    {
    case 0:
        return &descriptor;
    default:
        return NULL;
    }
}
