// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include <m_pd.h>
#include <sfizz.h>
#include <spin_mutex.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static t_class* cls_sfizz_tilde;

typedef struct _sfizz_tilde {
    t_object obj;
    t_outlet* outputs[2];
    sfizz_synth_t* synth;
    spin_mutex_t* mutex;
    int midi[3];
    int midinum;
    t_symbol* dir;
    char* filepath;
} t_sfizz_tilde;

static void sfizz_tilde_set_file(t_sfizz_tilde* self, const char* file)
{
    const char* dir = self->dir->s_name;
    char* filepath;
    if (file[0] != '\0') {
        filepath = malloc(strlen(dir) + 1 + strlen(file) + 1);
        sprintf(filepath, "%s/%s", dir, file);
    }
    else {
        filepath = malloc(1);
        *filepath = '\0';
    }
    free(self->filepath);
    self->filepath = filepath;
}

static bool sfizz_tilde_do_load(t_sfizz_tilde* self)
{
    bool loaded;
    spin_mutex_lock(self->mutex);
    if (self->filepath[0] != '\0')
        loaded = sfizz_load_file(self->synth, self->filepath);
    else
        loaded = sfizz_load_string(self->synth, "default.sfz", "<region>sample=*sine");
    spin_mutex_unlock(self->mutex);
    return loaded;
}

static void* sfizz_tilde_new(t_symbol* sym, int argc, t_atom argv[])
{
    (void)sym;
    t_sfizz_tilde* self = (t_sfizz_tilde*)pd_new(cls_sfizz_tilde);

    const char* file;
    if (argc == 0)
        file = "";
    else if (argc == 1 && argv[0].a_type == A_SYMBOL)
        file = argv[0].a_w.w_symbol->s_name;
    else {
        pd_free((t_pd*)self);
        return NULL;
    }

    self->dir = canvas_getcurrentdir();

    self->outputs[0] = outlet_new(&self->obj, &s_signal);
    self->outputs[1] = outlet_new(&self->obj, &s_signal);

    sfizz_synth_t* synth = sfizz_create_synth();
    self->synth = synth;

    self->mutex = spin_mutex_create();

    sfizz_set_sample_rate(synth, sys_getsr());
    sfizz_set_samples_per_block(synth, sys_getblksize());

    sfizz_tilde_set_file(self, file);
    if (!sfizz_tilde_do_load(self)) {
        pd_free((t_pd*)self);
        return NULL;
    }

    return self;
}

static void sfizz_tilde_free(t_sfizz_tilde* self)
{
    if (self->filepath)
        free(self->filepath);
    if (self->synth)
        sfizz_free(self->synth);
    if (self->mutex)
        spin_mutex_destroy(self->mutex);
    if (self->outputs[0])
        outlet_free(self->outputs[0]);
    if (self->outputs[1])
        outlet_free(self->outputs[1]);
}

static t_int* sfizz_tilde_perform(t_int* w)
{
    t_sfizz_tilde* self;
    t_sample* outputs[2];
    t_int nframes;

    w++;
    self = (t_sfizz_tilde*)*w++;
    outputs[0] = (t_sample*)*w++;
    outputs[1] = (t_sample*)*w++;
    nframes = (t_int)*w++;

    if (spin_mutex_trylock(self->mutex)) {
        sfizz_render_block(self->synth, outputs, 2, nframes);
        spin_mutex_unlock(self->mutex);
    }
    else {
        memset(outputs[0], 0, nframes * sizeof(t_sample));
        memset(outputs[1], 0, nframes * sizeof(t_sample));
    }

    return w;
}

static void sfizz_tilde_dsp(t_sfizz_tilde* self, t_signal** sp)
{
    dsp_add(
        &sfizz_tilde_perform, 4, (t_int)self,
        (t_int)sp[0]->s_vec, (t_int)sp[1]->s_vec, (t_int)sp[0]->s_n);
}

static void sfizz_tilde_midiin(t_sfizz_tilde* self, t_float f)
{
    int byte = (int)f;
    bool isstatus = (byte & 0x80) != 0;

    int* midi = self->midi;
    int midinum = self->midinum;

    //
    if (isstatus) {
        midi[0] = byte;
        midinum = 1;
    }
    else if (midinum != -1 && midinum < 3)
        midi[midinum++] = byte;
    else
        midinum = -1;

    //
    switch (midinum) {
    case 2:
        switch (midi[0] & 0xf0) {
        case 0xd0: // channel aftertouch
            if (spin_mutex_trylock(self->mutex)) {
                sfizz_send_channel_aftertouch(self->synth, 0, midi[1]);
                spin_mutex_unlock(self->mutex);
            }
            break;
        }
        break;
    case 3:
        switch (midi[0] & 0xf0) {
        case 0x90: // note on
            if (midi[2] == 0)
                goto noteoff;
            if (spin_mutex_trylock(self->mutex)) {
                sfizz_send_note_on(self->synth, 0, midi[1], midi[2]);
                spin_mutex_unlock(self->mutex);
            }
            break;
        case 0x80: // note off
        noteoff:
            if (spin_mutex_trylock(self->mutex)) {
                sfizz_send_note_off(self->synth, 0, midi[1], midi[2]);
                spin_mutex_unlock(self->mutex);
            }
            break;
        case 0xb0: // controller
            if (spin_mutex_trylock(self->mutex)) {
                sfizz_send_cc(self->synth, 0, midi[1], midi[2]);
                spin_mutex_unlock(self->mutex);
            }
            break;
        case 0xa0: // key aftertouch
            if (spin_mutex_trylock(self->mutex)) {
                sfizz_send_poly_aftertouch(self->synth, 0, midi[1], midi[2]);
                spin_mutex_unlock(self->mutex);
            }
            break;
        case 0xe0: // pitch bend
            if (spin_mutex_trylock(self->mutex)) {
                sfizz_send_pitch_wheel(self->synth, 0, (midi[1] + (midi[2] << 7)) - 8192);
                spin_mutex_unlock(self->mutex);
            }
            break;

        }
        break;
    }

    self->midinum = midinum;
}

static void sfizz_tilde_load(t_sfizz_tilde* self, t_symbol* sym)
{
    sfizz_tilde_set_file(self, sym->s_name);
    sfizz_tilde_do_load(self);
}

static void sfizz_tilde_reload(t_sfizz_tilde* self, t_float value)
{
    (void)value;
    sfizz_tilde_do_load(self);
}

static void sfizz_tilde_hdcc(t_sfizz_tilde* self, t_float cc, t_float value)
{
    if (spin_mutex_trylock(self->mutex)) {
        sfizz_automate_hdcc(self->synth, 0, (int)cc, value);
        spin_mutex_unlock(self->mutex);
    }
}

static void sfizz_tilde_voices(t_sfizz_tilde* self, t_float value)
{
    int numvoices = (int)value;
    if (numvoices < 1)
        numvoices = 1;

    spin_mutex_lock(self->mutex);
    sfizz_set_num_voices(self->synth, numvoices);
    spin_mutex_unlock(self->mutex);
}

#if defined(_WIN32)
__declspec(dllexport)
#else
__attribute__((visibility("default")))
#endif
void sfizz_setup()
{
    post("sfizz external for Puredata");

    cls_sfizz_tilde = class_new(
        gensym("sfizz~"),
        (t_newmethod)&sfizz_tilde_new,
        (t_method)&sfizz_tilde_free,
        sizeof(t_sfizz_tilde),
        CLASS_DEFAULT, A_GIMME, A_NULL);
    class_addmethod(
        cls_sfizz_tilde, (t_method)&sfizz_tilde_dsp, gensym("dsp"), A_CANT, A_NULL);
    class_addmethod(
        cls_sfizz_tilde, (t_method)&sfizz_tilde_midiin, &s_float, A_FLOAT, A_NULL);
    class_addmethod(
        cls_sfizz_tilde, (t_method)&sfizz_tilde_load, gensym("load"), A_DEFSYM, A_NULL);
    class_addmethod(
        cls_sfizz_tilde, (t_method)&sfizz_tilde_reload, gensym("reload"), A_DEFFLOAT, A_NULL);
    class_addmethod(
        cls_sfizz_tilde, (t_method)&sfizz_tilde_hdcc, gensym("hdcc"), A_FLOAT, A_FLOAT, A_NULL);
    class_addmethod(
        cls_sfizz_tilde, (t_method)&sfizz_tilde_voices, gensym("voices"), A_FLOAT, A_NULL);
}
