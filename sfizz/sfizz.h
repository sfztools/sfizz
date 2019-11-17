// Copyright (c) 2019, Paul Ferrand
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sfizz_synth_t sfizz_synth_t;

sfizz_synth_t* sfizz_create_synth();
void sfizz_free(sfizz_synth_t* synth);

bool sfizz_load_file(sfizz_synth_t* synth, char* path);

int sfizz_get_num_regions(sfizz_synth_t* synth);
int sfizz_get_num_groups(sfizz_synth_t* synth);
int sfizz_get_num_masters(sfizz_synth_t* synth);
int sfizz_get_num_curves(sfizz_synth_t* synth);
int sfizz_get_num_preloaded_samples(sfizz_synth_t* synth);
int sfizz_get_num_active_voices(sfizz_synth_t* synth);

void sfizz_set_samples_per_block(sfizz_synth_t* synth, int samples_per_block);
void sfizz_set_sample_rate(sfizz_synth_t* synth, float sample_rate);

void sfizz_send_note_on(sfizz_synth_t* synth, int delay, int channel, int note_number, char velocity);
void sfizz_send_note_off(sfizz_synth_t* synth, int delay, int channel, int note_number, char velocity);
void sfizz_send_cc(sfizz_synth_t* synth, int delay, int channel, int cc_number, char cc_value);
void sfizz_send_pitch_wheel(sfizz_synth_t* synth, int delay, int channel, int pitch);
void sfizz_send_aftertouch(sfizz_synth_t* synth, int delay, int channel, char aftertouch);
void sfizz_send_tempo(sfizz_synth_t* synth, int delay, float seconds_per_quarter);

void sfizz_render_block(sfizz_synth_t* synth, float** channels, int num_channels, int num_frames);

void sfizz_force_garbage_collection(sfizz_synth_t* synth);

#ifdef __cplusplus
}
#endif