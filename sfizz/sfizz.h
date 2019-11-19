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

/**
  @file
  @brief sfizz public API.
*/

#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct sfizz_synth_t sfizz_synth_t;

/**
 * @brief Creates a sfizz synth.
 * This object has to be freed by the caller using sfizz_free().
 * 
 * @return sfizz_synth_t* 
 */
sfizz_synth_t* sfizz_create_synth();
/**
 * @brief Frees an existing sfizz synth.
 * 
 * @param synth The synth to destroy
 */
void sfizz_free(sfizz_synth_t* synth);

/**
 * @brief Loads an SFZ file.
 * The file path can be absolute or relative. All file operations for this SFZ file will be relative to the parent directory of the SFZ file.
 * 
 * @param synth The sfizz synth.
 * @param path A null-terminated string representing a path to an SFZ file.
 * @return true when file loading went OK.
 * @return false if some error occured while loading.
 */
bool sfizz_load_file(sfizz_synth_t* synth, char* path);

/**
 * @brief Returns the number of regions in the currently loaded SFZ file.
 * 
 * @param synth 
 * @return int the number of regions
 */
int sfizz_get_num_regions(sfizz_synth_t* synth);
/**
 * @brief Returns the number of groups in the currently loaded SFZ file.
 * 
 * @param synth 
 * @return int the number of groups
 */
int sfizz_get_num_groups(sfizz_synth_t* synth);
/**
 * @brief Returns the number of masters in the currently loaded SFZ file.
 * 
 * @param synth 
 * @return int the number of masters
 */
int sfizz_get_num_masters(sfizz_synth_t* synth);
/**
 * @brief Returns the number of curves in the currently loaded SFZ file.
 * 
 * @param synth 
 * @return int the number of curves
 */
int sfizz_get_num_curves(sfizz_synth_t* synth);
/**
 * @brief Returns the number of preloaded samples for the current SFZ file.
 * 
 * @param synth 
 * @return int the number of preloaded samples
 */
int sfizz_get_num_preloaded_samples(sfizz_synth_t* synth);
/**
 * @brief Returns the number of active voices.
 * Note that this function is a basic indicator and does not aim to be perfect.
 * In particular, it runs on the calling thread so voices may well start or stop
 * while the function is checking which voice is active.
 * 
 * @param synth 
 * @return int the number of playing voices
 */
int sfizz_get_num_active_voices(sfizz_synth_t* synth);

/**
 * @brief Sets the expected number of samples per block.
 * If unsure, give an upper bound since right now ugly things may happen if you
 * go over this number.
 * 
 * @param synth 
 * @param samples_per_block the number of samples per block
 */
void sfizz_set_samples_per_block(sfizz_synth_t* synth, int samples_per_block);
/**
 * @brief Sets the sample rate for the synth.
 * This is the output sample rate. This setting does not affect the internal
 * processing.
 * 
 * @param synth 
 * @param sample_rate the sample rate
 */
void sfizz_set_sample_rate(sfizz_synth_t* synth, float sample_rate);

/**
 * @brief Send a note on event to the synth.
 * As with all MIDI events, this needs to happen before the call to sfizz_render_block
 * in each block.
 * 
 * @param synth 
 * @param delay the delay of the event in the block, in samples.
 * @param channel the MIDI channel; currently starts at 1 (FIXME)
 * @param note_number the MIDI note number
 * @param velocity the MIDI velocity
 */
void sfizz_send_note_on(sfizz_synth_t* synth, int delay, int channel, int note_number, char velocity);

/**
 * @brief Send a note off event to the synth.
 * As with all MIDI events, this needs to happen before the call to sfizz_render_block
 * in each block. As per the SFZ spec the velocity of note-off events is usually replaced
 * by the note-on velocity.
 * 
 * @param synth 
 * @param delay the delay of the event in the block, in samples.
 * @param channel the MIDI channel; currently starts at 1 (FIXME)
 * @param note_number the MIDI note number
 * @param velocity the MIDI velocity
 */
void sfizz_send_note_off(sfizz_synth_t* synth, int delay, int channel, int note_number, char velocity);

/**
 * @brief Send a CC event to the synth.
 * As with all MIDI events, this needs to happen before the call to sfizz_render_block
 * in each block.
 * 
 * @param synth 
 * @param delay the delay of the event in the block, in samples.
 * @param channel the MIDI channel; currently starts at 1 (FIXME)
 * @param cc_number the MIDI CC number
 * @param cc_value the MIDI CC value
 */
void sfizz_send_cc(sfizz_synth_t* synth, int delay, int channel, int cc_number, char cc_value);
/**
 * @brief Send a pitch wheel event. (CURRENTLY UNIMPLEMENTED)
 * 
 * @param synth 
 * @param delay 
 * @param channel 
 * @param pitch 
 */
void sfizz_send_pitch_wheel(sfizz_synth_t* synth, int delay, int channel, int pitch);

/**
 * @brief Send an aftertouch event. (CURRENTLY UNIMPLEMENTED)
 * 
 * @param synth 
 * @param delay 
 * @param channel 
 * @param aftertouch 
 */
void sfizz_send_aftertouch(sfizz_synth_t* synth, int delay, int channel, char aftertouch);

/**
 * @brief Send a tempo event. (CURRENTLY UNIMPLEMENTED)
 * 
 * @param synth 
 * @param delay 
 * @param seconds_per_quarter 
 */
void sfizz_send_tempo(sfizz_synth_t* synth, int delay, float seconds_per_quarter);

/**
 * @brief Render a block audio data into a stereo channel.
 * No other channel configuration is supported. The synth will gracefully ignore your request if you
 * provide a value. You should pass all the relevant events for the block (midi notes, CCs, ...) before
 * rendering each block. The synth will memorize the inputs and render sample accurates envelopes
 * depending on the input events passed to it.
 * 
 * @param synth 
 * @param channels pointers to the left and right channel of the output
 * @param num_channels should be equal to 2 for the time being.
 * @param num_frames number of frames to fill. This should be less than or equal to the expected
 *      samples_per_block.
 */
void sfizz_render_block(sfizz_synth_t* synth, float** channels, int num_channels, int num_frames);

/**
 * @brief Force a memory cleanup of the samples loaded in the background.
 * This should happen automatically but if you want it done more frequently
 * this is the way to go.
 * 
 * @param synth 
 */
void sfizz_force_garbage_collection(sfizz_synth_t* synth);

#ifdef __cplusplus
}
#endif