/*
  Sfizz LV2 plugin

  Copyright 2019, Paul Ferrand <paul@ferrand.cc>

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

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

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

#include <math.h>
#include <sfizz.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_SFZ_FILE "/home/paul/Documents/AVL_Percussions/AVL_Drumkits_Percussion-1.0-Alt.sfz"
#define SFIZZ_URI "http://sfztools.github.io/sfizz"
#define SFIZZ_PREFIX SFIZZ_URI "#"
#define SFIZZ__sfzFile "http://sfztools.github.io/sfizz:sfzfile"
#define SFIZZ__numVoices "http://sfztools.github.io/sfizz:numvoices"
#define SFIZZ__preloadSize "http://sfztools.github.io/sfizz:preload_size"
#define SFIZZ__oversampling "http://sfztools.github.io/sfizz:oversampling"
// This one is just for the worker
#define SFIZZ__logStatus "http://sfztools.github.io/sfizz:log_status"

#define CHANNEL_MASK 0x0F
#define MIDI_CHANNEL(byte) (byte & CHANNEL_MASK)
#define MIDI_STATUS(byte) (byte & ~CHANNEL_MASK)
#define MAX_BLOCK_SIZE 8192
#define MAX_PATH_SIZE 1024
#define MAX_VOICES 256
#define DEFAULT_VOICES 64
#define DEFAULT_OVERSAMPLING SFIZZ_OVERSAMPLING_X1
#define DEFAULT_PRELOAD 8192
#define LOG_SAMPLE_COUNT 96000
#define UNUSED(x) (void)(x)

typedef struct
{
    // Features
    LV2_URID_Map *map;
    LV2_URID_Unmap *unmap;
    LV2_Worker_Schedule *worker;
    LV2_Log_Log *log;

    // Ports
    const LV2_Atom_Sequence *control_port;
    LV2_Atom_Sequence *notify_port;
    float *output_buffers[2];
    const float *volume_port;
    const float *polyphony_port;
    const float *oversampling_port;
    const float *preload_port;
    const float *freewheel_port;

    // Atom forge
    LV2_Atom_Forge forge;              ///< Forge for writing atoms in run thread
    LV2_Atom_Forge_Frame notify_frame; ///< Cached for worker replies

    // Logger
    LV2_Log_Logger logger;

    // URIs
    LV2_URID midi_event_uri;
    LV2_URID options_interface_uri;
    LV2_URID max_block_length_uri;
    LV2_URID nominal_block_length_uri;
    LV2_URID sample_rate_uri;
    LV2_URID atom_object_uri;
    LV2_URID atom_float_uri;
    LV2_URID atom_int_uri;
    LV2_URID atom_urid_uri;
    LV2_URID atom_path_uri;
    LV2_URID patch_set_uri;
    LV2_URID patch_get_uri;
    LV2_URID patch_put_uri;
    LV2_URID patch_property_uri;
    LV2_URID patch_value_uri;
    LV2_URID patch_body_uri;
    LV2_URID state_changed_uri;
    LV2_URID sfizz_sfz_file_uri;
    LV2_URID sfizz_num_voices_uri;
    LV2_URID sfizz_preload_size_uri;
    LV2_URID sfizz_oversampling_uri;
    LV2_URID sfizz_log_status_uri;

    // Sfizz related data
    sfizz_synth_t *synth;
    bool expect_nominal_block_length;
    char sfz_file_path[MAX_PATH_SIZE];
    int num_voices;
    unsigned int preload_size;
    sfizz_oversampling_factor_t oversampling;
    bool changing_state;
    int max_block_size;
    int sample_counter;
    float sample_rate;
} sfizz_plugin_t;

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
    SFIZZ_FREEWHEELING = 8
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
    self->atom_int_uri = map->map(map->handle, LV2_ATOM__Int);
    self->atom_path_uri = map->map(map->handle, LV2_ATOM__Path);
    self->atom_urid_uri = map->map(map->handle, LV2_ATOM__URID);
    self->atom_object_uri = map->map(map->handle, LV2_ATOM__Object);
    self->patch_set_uri = map->map(map->handle, LV2_PATCH__Set);
    self->patch_get_uri = map->map(map->handle, LV2_PATCH__Get);
    self->patch_put_uri = map->map(map->handle, LV2_PATCH__Put);
    self->patch_body_uri = map->map(map->handle, LV2_PATCH__body);
    self->patch_property_uri = map->map(map->handle, LV2_PATCH__property);
    self->patch_value_uri = map->map(map->handle, LV2_PATCH__value);
    self->state_changed_uri = map->map(map->handle, LV2_STATE__StateChanged);
    self->sfizz_sfz_file_uri = map->map(map->handle, SFIZZ__sfzFile);
    self->sfizz_num_voices_uri = map->map(map->handle, SFIZZ__numVoices);
    self->sfizz_preload_size_uri = map->map(map->handle, SFIZZ__preloadSize);
    self->sfizz_oversampling_uri = map->map(map->handle, SFIZZ__oversampling);
    self->sfizz_log_status_uri = map->map(map->handle, SFIZZ__logStatus);
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
    default:
        break;
    }
}

static LV2_Handle
instantiate(const LV2_Descriptor *descriptor,
            double rate,
            const char *path,
            const LV2_Feature *const *features)
{
    UNUSED(descriptor);
    UNUSED(path);
    LV2_Options_Option *options = NULL;
    bool supports_bounded_block_size = false;
    bool options_has_block_size = false;
    bool supports_fixed_block_size = false;

    // Allocate and initialise instance structure.
    sfizz_plugin_t *self = (sfizz_plugin_t *)calloc(1, sizeof(sfizz_plugin_t));
    if (!self)
        return NULL;

    // Set defaults
    self->max_block_size = MAX_BLOCK_SIZE;
    self->sample_rate = (float)rate;
    self->expect_nominal_block_length = false;
    self->sfz_file_path[0] = '\0';
    self->num_voices = DEFAULT_VOICES;
    self->oversampling = DEFAULT_OVERSAMPLING;
    self->preload_size = DEFAULT_PRELOAD;
    self->changing_state = false;
    self->sample_counter = 0;

    // Get the features from the host and populate the structure
    for (const LV2_Feature *const *f = features; *f; f++)
    {
        // lv2_log_note(&self->logger, "Feature URI: %s\n", (**f).URI);

        if (!strcmp((**f).URI, LV2_URID__map))
            self->map = (**f).data;

        if (!strcmp((**f).URI, LV2_URID__unmap))
            self->unmap = (**f).data;

        if (!strcmp((**f).URI, LV2_BUF_SIZE__boundedBlockLength))
            supports_bounded_block_size = true;

        if (!strcmp((**f).URI, LV2_BUF_SIZE__fixedBlockLength))
            supports_fixed_block_size = true;

        if (!strcmp((**f).URI, LV2_OPTIONS__options))
            options = (**f).data;

        if (!strcmp((**f).URI, LV2_WORKER__schedule))
            self->worker = (**f).data;

        if (!strcmp((**f).URI, LV2_LOG__log))
            self->log = (**f).data;
    }

    // Setup the loggers
    lv2_log_logger_init(&self->logger, self->map, self->log);

    // The map feature is required
    if (!self->map)
    {
        lv2_log_error(&self->logger, "Map feature not found, aborting...\n");
        free(self);
        return NULL;
    }

    // The worker feature is required
    if (!self->worker)
    {
        lv2_log_error(&self->logger, "Worker feature not found, aborting...\n");
        free(self);
        return NULL;
    }

    // Map the URIs we will need
    sfizz_lv2_map_required_uris(self);

    // Initialize the forge
    lv2_atom_forge_init(&self->forge, self->map);

    // Check the options for the block size and sample rate parameters
    if (options)
    {
        for (const LV2_Options_Option *opt = options; opt->value; ++opt)
        {
            if (opt->key == self->sample_rate_uri)
            {
                if (opt->type != self->atom_float_uri)
                {
                    lv2_log_warning(&self->logger, "Got a sample rate but the type was wrong\n");
                    continue;
                }
                self->sample_rate = *(float *)opt->value;
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
                      "Bounded block size not supported and options gave no block size, aborting...\n");
        free(self);
        return NULL;
    }

    return (LV2_Handle)self;
}

static void
cleanup(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    free(self);
}

static void
activate(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    self->synth = sfizz_create_synth();
    sfizz_set_samples_per_block(self->synth, self->max_block_size);
    sfizz_set_sample_rate(self->synth, self->sample_rate);
    if (strlen(self->sfz_file_path) > 0)
    {
        lv2_log_note(&self->logger, "Current file is: %s\n", self->sfz_file_path);
        sfizz_load_file(self->synth, self->sfz_file_path);
    }
}

static void
deactivate(LV2_Handle instance)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    sfizz_free(self->synth);
}

static void
sfizz_lv2_handle_atom_object(sfizz_plugin_t *self, const LV2_Atom_Object *obj)
{
    const LV2_Atom *property = NULL;
    lv2_atom_object_get(obj, self->patch_property_uri, &property, 0);
    if (!property)
    {
        lv2_log_error(&self->logger,
                      "[handle_object] Could not get the property from the patch object, aborting.\n");
        return;
    }

    if (property->type != self->atom_urid_uri)
    {
        lv2_log_error(&self->logger,
                      "[handle_object] Atom type was not a URID, aborting.\n");
        return;
    }

    const uint32_t key = ((const LV2_Atom_URID *)property)->body;
    const LV2_Atom *atom = NULL;
    lv2_atom_object_get(obj, self->patch_value_uri, &atom, 0);
    if (!atom)
    {
        lv2_log_error(&self->logger, "[handle_object] Error retrieving the atom, aborting.\n");
        if (self->unmap)
            lv2_log_warning(&self->logger,
                            "Atom URI: %s\n",
                            self->unmap->unmap(self->unmap->handle, key));
        return;
    }

    if (key == self->sfizz_sfz_file_uri)
    {

        const uint32_t original_atom_size = lv2_atom_total_size((const LV2_Atom *)atom);
        const uint32_t null_terminated_atom_size = original_atom_size + 1;
        char atom_buffer[null_terminated_atom_size];
        memcpy(&atom_buffer, atom, original_atom_size);
        atom_buffer[original_atom_size] = 0; // Null terminate the string for safety
        LV2_Atom *sfz_file_path = (LV2_Atom *)&atom_buffer;
        sfz_file_path->type = self->sfizz_sfz_file_uri;

        // If the parameter is different from the current one we send it through
        if (strcmp(self->sfz_file_path, LV2_ATOM_BODY_CONST(sfz_file_path)))
            self->worker->schedule_work(self->worker->handle, null_terminated_atom_size, sfz_file_path);
        lv2_log_note(&self->logger, "[handle_object] Received an SFZ file: %s\n", (char*)LV2_ATOM_BODY_CONST(sfz_file_path));
    }
    else
    {
        lv2_log_warning(&self->logger, "[handle_object] Unknown or unsupported object.\n");
        if (self->unmap)
            lv2_log_warning(&self->logger,
                            "Object URI: %s\n",
                            self->unmap->unmap(self->unmap->handle, key));
        return;
    }

    // Pfiou, we got a valid parameter, so send it to the worker
}

static void
sfizz_lv2_process_midi_event(sfizz_plugin_t *self, const LV2_Atom_Event *ev)
{
    const uint8_t *const msg = (const uint8_t *)(ev + 1);
    switch (lv2_midi_message_type(msg))
    {
    case LV2_MIDI_MSG_NOTE_ON:
        // lv2_log_note(&self->logger,
        //              "[process_midi] Received note on %d/%d at time %d\n", msg[0], msg[1], ev->time.frames);
        sfizz_send_note_on(self->synth,
                           (int)ev->time.frames,
                           (int)MIDI_CHANNEL(msg[0]) + 1,
                           (int)msg[1],
                           msg[2]);
        break;
    case LV2_MIDI_MSG_NOTE_OFF:
        // lv2_log_note(&self->logger,
        //              "[process_midi] Received note off %d/%d at time %d\n", msg[0], msg[1], ev->time.frames);
        sfizz_send_note_off(self->synth,
                            (int)ev->time.frames,
                            (int)MIDI_CHANNEL(msg[0]) + 1,
                            (int)msg[1],
                            msg[2]);
        break;
    case LV2_MIDI_MSG_CONTROLLER:
        // lv2_log_note(&self->logger,
        //              "[process_midi] Received CC %d/%d at time %d\n", msg[0], msg[1], ev->time.frames);
        sfizz_send_cc(self->synth,
                      (int)ev->time.frames,
                      (int)MIDI_CHANNEL(msg[0]) + 1,
                      (int)msg[1],
                      msg[2]);
        break;
    default:
        break;
    }
}

static void sfizz_lv2_send_file_path(sfizz_plugin_t *self)
{
    LV2_Atom_Forge_Frame frame;
    lv2_atom_forge_frame_time(&self->forge, 0);
    lv2_atom_forge_object(&self->forge, &frame, 0, self->patch_set_uri);
    lv2_atom_forge_key(&self->forge, self->patch_property_uri);
    lv2_atom_forge_urid(&self->forge, self->sfizz_sfz_file_uri);
    lv2_atom_forge_key(&self->forge, self->patch_value_uri);
    lv2_atom_forge_path(&self->forge, self->sfz_file_path, strlen(self->sfz_file_path));
    lv2_atom_forge_pop(&self->forge, &frame);
}

static void
sfizz_lv2_status_log(sfizz_plugin_t* self)
{
    lv2_log_note(&self->logger, "[run] Allocated buffers: %d\n", sfizz_get_num_buffers(self->synth));
    lv2_log_note(&self->logger, "[run] Allocated bytes: %d\n", sfizz_get_num_bytes(self->synth));
}

static void
run(LV2_Handle instance, uint32_t sample_count)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    if (!self->control_port || !self->notify_port)
        return;

    // Enable freewheeling on the synth if necessary, which wait for
    // the background loading queues to flush before rendering.
    if (*(self->freewheel_port) > 0) {
        sfizz_enable_freewheeling(self->synth);
    } else {
        sfizz_disable_freewheeling(self->synth);
    }

    // Set up forge to write directly to notify output port.
    const size_t notify_capacity = self->notify_port->atom.size;
    lv2_atom_forge_set_buffer(&self->forge, (uint8_t *)self->notify_port, notify_capacity);

    // Start a sequence in the notify output port.
    lv2_atom_forge_sequence_head(&self->forge, &self->notify_frame, 0);

    LV2_ATOM_SEQUENCE_FOREACH(self->control_port, ev)
    {
        // If the received atom is an object/patch message
        if (ev->body.type == self->atom_object_uri)
        {
            const LV2_Atom_Object *obj = (const LV2_Atom_Object *)&ev->body;
            if (obj->body.otype == self->patch_set_uri)
            {
                sfizz_lv2_handle_atom_object(self, obj);
            }
            else if (obj->body.otype == self->patch_get_uri)
            {
                const LV2_Atom_URID *property = NULL;
                lv2_atom_object_get(obj, self->patch_property_uri, &property, 0);
                if (!property) // Send the full state
                {
                    sfizz_lv2_send_file_path(self);
                }
                else if (property->body == self->sfizz_sfz_file_uri)
                {
                    sfizz_lv2_send_file_path(self);
                }
            }
            else
            {
                lv2_log_warning(&self->logger, "Got an Object atom but it was not supported.\n");
                if (self->unmap)
                    lv2_log_warning(&self->logger,
                                    "Object URI: %s\n",
                                    self->unmap->unmap(self->unmap->handle, obj->body.otype));
                continue;
            }
            // Got an atom that is a MIDI event
        }
        else if (ev->body.type == self->midi_event_uri)
        {
            sfizz_lv2_process_midi_event(self, ev);
        }
    }

    float volume = *(self->volume_port);
    sfizz_set_volume(self->synth, volume);

    int num_voices = (int)*self->polyphony_port;
    if (num_voices != self->num_voices && !self->changing_state) {
        lv2_log_note(&self->logger, "[run] Number of voices changed to %d\n", num_voices);
        LV2_Atom_Int num_voices_atom;
        num_voices_atom.atom.type = self->sfizz_num_voices_uri;
        num_voices_atom.atom.size = sizeof(int);
        num_voices_atom.body = num_voices;
        if (self->worker->schedule_work(self->worker->handle,
                                    lv2_atom_total_size((LV2_Atom*)&num_voices_atom),
                                    &num_voices_atom) == LV2_WORKER_SUCCESS)
        {
            self->changing_state = true;
        }
    }

    unsigned int preload_size = (int)*self->preload_port;
    if (preload_size != self->preload_size && !self->changing_state) {
        lv2_log_note(&self->logger, "[run] Preload size changed to %d\n", preload_size);
        LV2_Atom_Int atom;
        atom.atom.type = self->sfizz_preload_size_uri;
        atom.atom.size = sizeof(int);
        atom.body = preload_size;
        if (self->worker->schedule_work(self->worker->handle,
                                    lv2_atom_total_size((LV2_Atom*)&atom),
                                    &atom) == LV2_WORKER_SUCCESS)
        {
            self->changing_state = true;
        }
    }

    sfizz_oversampling_factor_t oversampling = (sfizz_oversampling_factor_t)*self->oversampling_port;
    if (oversampling != self->oversampling && !self->changing_state) {
        lv2_log_note(&self->logger, "[run] Oversampling size changed to %d\n", oversampling);
        LV2_Atom_Int atom;
        atom.atom.type = self->sfizz_oversampling_uri;
        atom.atom.size = sizeof(int);
        atom.body = oversampling;
        if (self->worker->schedule_work(self->worker->handle,
                                    lv2_atom_total_size((LV2_Atom*)&atom),
                                    &atom) == LV2_WORKER_SUCCESS)
        {
            self->changing_state = true;
        }
    }

#ifndef NDEBUG
    // Log the buffer usage
    self->sample_counter += (int)sample_count;
    if (self->sample_counter > LOG_SAMPLE_COUNT)
    {
        LV2_Atom atom;
        atom.type = self->sfizz_log_status_uri;
        atom.size = 0;
        if (!self->worker->schedule_work(self->worker->handle,
                                    lv2_atom_total_size((LV2_Atom*)&atom),
                                    &atom) == LV2_WORKER_SUCCESS)
        {
            lv2_log_error(&self->logger, "[run] There was an issue sending a logging message to the background worker");
        }
        self->sample_counter -= LOG_SAMPLE_COUNT;
    }
#endif

    // Render the block
    sfizz_render_block(self->synth, self->output_buffers, 2, (int)sample_count);
}

static uint32_t
lv2_get_options(LV2_Handle instance, LV2_Options_Option *options)
{
    UNUSED(instance);
    UNUSED(options);
    // We have no options
    return LV2_OPTIONS_ERR_UNKNOWN;
}

static uint32_t
lv2_set_options(LV2_Handle instance, const LV2_Options_Option *options)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    // Update the block size and sample rate as needed
    for (const LV2_Options_Option *opt = options; opt->value; ++opt)
    {
        if (opt->key == self->sample_rate_uri)
        {
            if (opt->type != self->atom_float_uri)
            {
                lv2_log_warning(&self->logger, "Got a sample rate but the type was wrong\n");
                continue;
            }
            self->sample_rate = *(float *)opt->value;
            sfizz_set_sample_rate(self->synth, self->sample_rate);
        }
        else if (!self->expect_nominal_block_length && opt->key == self->max_block_length_uri)
        {
            if (opt->type != self->atom_int_uri)
            {
                lv2_log_warning(&self->logger, "Got a max block size but the type was wrong\n");
                continue;
            }
            self->max_block_size = *(int *)opt->value;
            sfizz_set_samples_per_block(self->synth, self->max_block_size);
        }
        else if (opt->key == self->nominal_block_length_uri)
        {
            if (opt->type != self->atom_int_uri)
            {
                lv2_log_warning(&self->logger, "Got a nominal block size but the type was wrong\n");
                continue;
            }
            self->max_block_size = *(int *)opt->value;
            sfizz_set_samples_per_block(self->synth, self->max_block_size);
        }
    }
    return LV2_OPTIONS_SUCCESS;
}

static LV2_State_Status
restore(LV2_Handle instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle handle,
        uint32_t flags,
        const LV2_Feature *const *features)
{
    UNUSED(flags);
    UNUSED(features);
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;

    // Fetch back the saved file path, if any
    size_t size;
    uint32_t type;
    uint32_t val_flags;
    const void *value;
    value = retrieve(handle, self->sfizz_sfz_file_uri, &size, &type, &val_flags);
    if (value)
    {
        lv2_log_note(&self->logger, "Restoring the file %s\n", (const char *)value);
        if (sfizz_load_file(self->synth, (const char *)value))
            strcpy(self->sfz_file_path, (const char *)value);
    }

    value = retrieve(handle, self->sfizz_num_voices_uri, &size, &type, &val_flags);
    if (value)
    {
        int num_voices = *(const int *)value;
        if (num_voices > 0 && num_voices <= MAX_VOICES && num_voices != self->num_voices)
        {
            lv2_log_note(&self->logger, "Restoring the number of voices to %d\n", num_voices);
            sfizz_set_num_voices(self->synth, num_voices);
            self->num_voices = num_voices;
        }
    }

    value = retrieve(handle, self->sfizz_preload_size_uri, &size, &type, &val_flags);
    if (value)
    {
        unsigned int preload_size = *(const unsigned int *)value;
        if (preload_size != self->preload_size)
        {
            lv2_log_note(&self->logger, "Restoring the preload size to %d\n", preload_size);
            sfizz_set_preload_size(self->synth, preload_size);
            self->preload_size = preload_size;
        }
    }

    value = retrieve(handle, self->sfizz_oversampling_uri, &size, &type, &val_flags);
    if (value)
    {
        sfizz_oversampling_factor_t oversampling = *(const sfizz_oversampling_factor_t *)value;
        if (oversampling != self->oversampling)
        {
            lv2_log_note(&self->logger, "Restoring the oversampling to %d\n", oversampling);
            sfizz_set_oversampling_factor(self->synth, oversampling);
            self->oversampling = oversampling;
        }
    }
    return LV2_STATE_SUCCESS;
}

static LV2_State_Status
save(LV2_Handle instance,
     LV2_State_Store_Function store,
     LV2_State_Handle handle,
     uint32_t flags,
     const LV2_Feature *const *features)
{
    UNUSED(flags);
    UNUSED(features);
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    // Save the file path
    store(handle,
          self->sfizz_sfz_file_uri,
          self->sfz_file_path,
          strlen(self->sfz_file_path) + 1,
          self->atom_path_uri,
          LV2_STATE_IS_POD);

    // Save the number of voices
    store(handle,
          self->sfizz_num_voices_uri,
          &self->num_voices,
          sizeof(int),
          self->atom_int_uri,
          LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    // Save the preload size
    store(handle,
          self->sfizz_preload_size_uri,
          &self->preload_size,
          sizeof(unsigned int),
          self->atom_int_uri,
          LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    // Save the preload size
    store(handle,
          self->sfizz_oversampling_uri,
          &self->oversampling,
          sizeof(int),
          self->atom_int_uri,
          LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

    return LV2_STATE_SUCCESS;
}

// This runs in a lower priority thread
static LV2_Worker_Status
work(LV2_Handle instance,
     LV2_Worker_Respond_Function respond,
     LV2_Worker_Respond_Handle handle,
     uint32_t size,
     const void *data)
{
    sfizz_plugin_t *self = (sfizz_plugin_t *)instance;
    if (!data)
    {
        lv2_log_error(&self->logger, "[worker] Got an empty data.\n");
        return LV2_WORKER_ERR_UNKNOWN;
    }

    const LV2_Atom *atom = (const LV2_Atom *)data;
    if (atom->type == self->sfizz_sfz_file_uri)
    {
        const char *sfz_file_path = LV2_ATOM_BODY_CONST(atom);
        lv2_log_note(&self->logger, "[work] Loading file: %s\n", sfz_file_path);
        sfizz_load_file(self->synth, sfz_file_path);
    }
    else if (atom->type == self->sfizz_num_voices_uri)
    {
        const int num_voices = *(const int *)LV2_ATOM_BODY_CONST(atom);
        lv2_log_note(&self->logger, "[work] Changing number of voices to: %d\n", num_voices);
        sfizz_set_num_voices(self->synth, num_voices);
    }
    else if (atom->type == self->sfizz_preload_size_uri)
    {
        const unsigned int preload_size = *(const unsigned int *)LV2_ATOM_BODY_CONST(atom);
        lv2_log_note(&self->logger, "[work] Changing preload size to: %d\n", preload_size);
        sfizz_set_preload_size(self->synth, preload_size);
    }
    else if (atom->type == self->sfizz_oversampling_uri)
    {
        const sfizz_oversampling_factor_t oversampling =
            *(const sfizz_oversampling_factor_t *)LV2_ATOM_BODY_CONST(atom);
        lv2_log_note(&self->logger, "[work] Changing oversampling to: %d\n", oversampling);
        sfizz_set_oversampling_factor(self->synth, oversampling);
    }
    else if (atom->type == self->sfizz_log_status_uri)
    {
        sfizz_lv2_status_log(self);
    }
    else
    {
        lv2_log_error(&self->logger, "[worker] Got an unknown atom.\n");
        if (self->unmap)
            lv2_log_error(&self->logger,
                          "URI: %s\n",
                          self->unmap->unmap(self->unmap->handle, atom->type));
        return LV2_WORKER_ERR_UNKNOWN;
    }

    respond(handle, size, data);
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
    if (atom->type == self->sfizz_sfz_file_uri)
    { // TODO: could check that everything is indeed changed here
        const char *sfz_file_path = LV2_ATOM_BODY_CONST(atom);
        strcpy(self->sfz_file_path, sfz_file_path);
        lv2_log_note(&self->logger, "[work_response] File changed to: %s\n", self->sfz_file_path);
    }
    else if (atom->type == self->sfizz_num_voices_uri)
    { // TODO: could check that everything is indeed changed here
        const int num_voices = *(const int *)LV2_ATOM_BODY_CONST(atom);
        self->num_voices = num_voices;
        self->changing_state = false;
        lv2_log_note(&self->logger, "[work_response] Number of voices changed to: %d\n", self->num_voices);
    }
    else if (atom->type == self->sfizz_preload_size_uri)
    { // TODO: could check that everything is indeed changed here
        const unsigned int preload_size = *(const unsigned int *)LV2_ATOM_BODY_CONST(atom);
        self->preload_size = preload_size;
        self->changing_state = false;
        lv2_log_note(&self->logger, "[work_response] Preload size changed to: %d\n", self->preload_size);
    }
    else if (atom->type == self->sfizz_oversampling_uri)
    { // TODO: could check that everything is indeed changed here
        const sfizz_oversampling_factor_t oversampling =
            *(const sfizz_oversampling_factor_t *)LV2_ATOM_BODY_CONST(atom);
        self->oversampling = oversampling;
        self->changing_state = false;
        lv2_log_note(&self->logger, "[work_response] Oversampling changed to: %d\n", self->oversampling);
    }
    else if (atom->type == self->sfizz_log_status_uri)
    {
        // Nothing to do
    }
    else
    {
        lv2_log_error(&self->logger, "[work_response] Got an unknown atom.\n");
        if (self->unmap)
            lv2_log_error(&self->logger,
                          "URI: %s\n",
                          self->unmap->unmap(self->unmap->handle, atom->type));
        return LV2_WORKER_ERR_UNKNOWN;
    }

    return LV2_WORKER_SUCCESS;
}

static const void *
extension_data(const char *uri)
{
    static const LV2_Options_Interface options = {lv2_get_options, lv2_set_options};
    static const LV2_State_Interface state = {save, restore};
    static const LV2_Worker_Interface worker = {work, work_response, NULL};

    // Advertise the extensions we support
    if (!strcmp(uri, LV2_OPTIONS__interface))
        return &options;
    else if (!strcmp(uri, LV2_STATE__interface))
        return &state;
    else if (!strcmp(uri, LV2_WORKER__interface))
        return &worker;

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
