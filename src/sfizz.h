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
  @brief sfizz public C API
*/

#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct sfizz_synth_t sfizz_synth_t;
typedef enum {
    SFIZZ_OVERSAMPLING_X1 = 1,
    SFIZZ_OVERSAMPLING_X2 = 2,
    SFIZZ_OVERSAMPLING_X4 = 4,
    SFIZZ_OVERSAMPLING_X8 = 8
} sfizz_oversampling_factor_t;

/**
 * @brief      Creates a sfizz synth. This object has to be freed by the caller
 *             using sfizz_free().
 *
 * @return     sfizz_synth_t*
 */
sfizz_synth_t* sfizz_create_synth();
/**
 * @brief      Frees an existing sfizz synth.
 *
 * @param      synth  The synth to destroy
 */
void sfizz_free(sfizz_synth_t* synth);

/**
 * @brief      Loads an SFZ file. The file path can be absolute or relative. All
 *             file operations for this SFZ file will be relative to the parent
 *             directory of the SFZ file.
 *
 * @param      synth  The sfizz synth.
 * @param      path   A null-terminated string representing a path to an SFZ
 *                    file.
 *
 * @return     true when file loading went OK.
 * @return     false if some error occured while loading.
 */
bool sfizz_load_file(sfizz_synth_t* synth, const char* path);

/**
 * @brief      Returns the number of regions in the currently loaded SFZ file.
 *
 * @param      synth  The synth
 *
 * @return     int the number of regions
 */
int sfizz_get_num_regions(sfizz_synth_t* synth);
/**
 * @brief      Returns the number of groups in the currently loaded SFZ file.
 *
 * @param      synth  The synth
 *
 * @return     int the number of groups
 */
int sfizz_get_num_groups(sfizz_synth_t* synth);
/**
 * @brief      Returns the number of masters in the currently loaded SFZ file.
 *
 * @param      synth  The synth
 *
 * @return     int the number of masters
 */
int sfizz_get_num_masters(sfizz_synth_t* synth);
/**
 * @brief      Returns the number of curves in the currently loaded SFZ file.
 *
 * @param      synth  The synth
 *
 * @return     int the number of curves
 */
int sfizz_get_num_curves(sfizz_synth_t* synth);
/**
 * @brief      Returns the number of preloaded samples for the current SFZ file.
 *
 * @param      synth  The synth
 *
 * @return     int the number of preloaded samples
 */
int sfizz_get_num_preloaded_samples(sfizz_synth_t* synth);
/**
 * @brief      Returns the number of active voices. Note that this function is a
 *             basic indicator and does not aim to be perfect. In particular, it
 *             runs on the calling thread so voices may well start or stop while
 *             the function is checking which voice is active.
 *
 * @param      synth  The synth
 *
 * @return     int the number of playing voices
 */
int sfizz_get_num_active_voices(sfizz_synth_t* synth);

/**
 * @brief      Sets the expected number of samples per block. If unsure, give an
 *             upper bound since right now ugly things may happen if you go over
 *             this number.
 *
 * @param      synth              The synth
 * @param      samples_per_block  the number of samples per block
 */
void sfizz_set_samples_per_block(sfizz_synth_t* synth, int samples_per_block);
/**
 * @brief      Sets the sample rate for the synth. This is the output sample
 *             rate. This setting does not affect the internal processing.
 *
 * @param      synth        The synth
 * @param      sample_rate  the sample rate
 */
void sfizz_set_sample_rate(sfizz_synth_t* synth, float sample_rate);

/**
 * @brief      Send a note on event to the synth. As with all MIDI events, this
 *             needs to happen before the call to sfizz_render_block in each
 *             block and should appear in order of the delays.
 *
 * @param      synth        The synth
 * @param      delay        the delay of the event in the block, in samples.
 * @param      note_number  the MIDI note number
 * @param      velocity     the MIDI velocity
 */
void sfizz_send_note_on(sfizz_synth_t* synth, int delay, int note_number, char velocity);

/**
 * @brief      Send a note off event to the synth. As with all MIDI events, this
 *             needs to happen before the call to sfizz_render_block in each
 *             block and should appear in order of the delays.
 *             As per the SFZ spec the velocity of note-off events is usually replaced by
 *             the note-on velocity.
 *
 * @param      synth        The synth
 * @param      delay        the delay of the event in the block, in samples.
 * @param      note_number  the MIDI note number
 * @param      velocity     the MIDI velocity
 */
void sfizz_send_note_off(sfizz_synth_t* synth, int delay, int note_number, char velocity);

/**
 * @brief      Send a CC event to the synth. As with all MIDI events, this needs
 *             to happen before the call to sfizz_render_block in each block and
 *             should appear in order of the delays.
 *
 * @param      synth      The synth
 * @param      delay      the delay of the event in the block, in samples.
 * @param      cc_number  the MIDI CC number
 * @param      cc_value   the MIDI CC value
 */
void sfizz_send_cc(sfizz_synth_t* synth, int delay, int cc_number, char cc_value);
/**
 * @brief      Send a pitch wheel event. As with all MIDI events, this needs
 *             to happen before the call to sfizz_render_block in each block and
 *             should appear in order of the delays.
 *
 * @param      synth    The synth
 * @param      delay    The delay
 * @param      pitch    The pitch
 */
void sfizz_send_pitch_wheel(sfizz_synth_t* synth, int delay, int pitch);

/**
 * @brief Send an aftertouch event. (CURRENTLY UNIMPLEMENTED)
 *
 * @param synth
 * @param delay
 * @param aftertouch
 */
void sfizz_send_aftertouch(sfizz_synth_t* synth, int delay, char aftertouch);

/**
 * @brief      Send a tempo event. (CURRENTLY UNIMPLEMENTED)
 *
 * @param      synth                The synth
 * @param      delay                The delay
 * @param      seconds_per_quarter  The seconds per quarter
 */
void sfizz_send_tempo(sfizz_synth_t* synth, int delay, float seconds_per_quarter);

/**
 * @brief      Render a block audio data into a stereo channel. No other channel
 *             configuration is supported. The synth will gracefully ignore your
 *             request if you provide a value. You should pass all the relevant
 *             events for the block (midi notes, CCs, ...) before rendering each
 *             block. The synth will memorize the inputs and render sample
 *             accurates envelopes depending on the input events passed to it.
 *
 * @param      synth         The synth
 * @param      channels      pointers to the left and right channel of the
 *                           output
 * @param      num_channels  should be equal to 2 for the time being.
 * @param      num_frames    number of frames to fill. This should be less than
 *                           or equal to the expected samples_per_block.
 */
void sfizz_render_block(sfizz_synth_t* synth, float** channels, int num_channels, int num_frames);

/**
 * @brief      Get the size of the preloaded data. This returns the number of
 *             floats used in the preloading buffers.
 *
 * @param      synth  The synth
 *
 * @return     the preloaded data size in sizeof(floats)
 */
unsigned int sfizz_get_preload_size(sfizz_synth_t* synth);
/**
 * @brief      Sets the size of the preloaded data in number of floats (not
 *             bytes). This will disable the callbacks for the duration of the
 *             load.
 *
 * @param      synth         The synth
 * @param[in]  preload_size  The preload size
 */
void sfizz_set_preload_size(sfizz_synth_t* synth, unsigned int preload_size);

/**
 * @brief      Get the internal oversampling rate. This is the sampling rate of
 *             the engine, not the output or expected rate of the calling
 *             function. For the latter use the `get_sample_rate()` functions.
 *
 * @param      synth  The synth
 *
 * @return     The internal sample rate of the engine
 */
sfizz_oversampling_factor_t sfizz_get_oversampling_factor(sfizz_synth_t* synth);
/**
 * @brief      Set the internal oversampling rate. This is the sampling rate of
 *             the engine, not the output or expected rate of the calling
 *             function. For the latter use the `set_sample_rate()` functions.
 *
 *             Increasing this value (up to x8 oversampling) improves the
 *             quality of the output at the expense of memory consumption and
 *             background loading speed. The main render path still uses the
 *             same linear interpolation algorithm and should not see its
 *             performance decrease, but the files are oversampled upon loading
 *             which increases the stress on the background loader and reduce
 *             the loading speed. You can tweak the size of the preloaded data
 *             to compensate for the memory increase, but the full loading will
 *             need to take place anyway.
 *
 * @param      synth         The synth
 * @param[in]  preload_size  The preload size
 *
 * @return     True if the oversampling factor was correct
 */
bool sfizz_set_oversampling_factor(sfizz_synth_t* synth, sfizz_oversampling_factor_t oversampling);

/**
 * @brief      Set the global instrument volume.
 *
 * @param      synth   The synth
 * @param      volume  the new volume
 */
void sfizz_set_volume(sfizz_synth_t* synth, float volume);

/**
 * @brief      Get the global instrument volume.
 *
 * @param      synth  The synth
 *
 * @return     float the instrument volume
 */
float sfizz_get_volume(sfizz_synth_t* synth);

/**
 * @brief      Sets the number of voices used by the synth
 *
 * @param      synth       The synth
 * @param      num_voices  The number voices
 */
void sfizz_set_num_voices(sfizz_synth_t* synth, int num_voices);
/**
 * @brief Returns the number of voices
 *
 * @param synth
 * @return num_voices
 */
int sfizz_get_num_voices(sfizz_synth_t* synth);

/**
 * @brief      Get the number of allocated buffers from the synth.
 *
 * @param      synth  The synth
 *
 * @return     The number of buffers held by the synth
 */
int sfizz_get_num_buffers(sfizz_synth_t* synth);
/**
 * @brief      Get the number of bytes allocated from the synth. Note that this
 *             value can be less than the actual memory usage since it only
 *             counts the buffer objects managed by sfizz.
 *
 * @param      synth  The synth
 *
 * @return     The number of bytes held by the synth in buffers;
 */
int sfizz_get_num_bytes(sfizz_synth_t* synth);

/**
 * @brief Enables freewheeling on the synth.
 *
 * @param synth
 */
void sfizz_enable_freewheeling(sfizz_synth_t* synth);
/**
 * @brief Disables freewheeling on the synth.
 *
 * @param synth
 */
void sfizz_disable_freewheeling(sfizz_synth_t* synth);
/**
 * @brief Get a comma separated list of unknown opcodes. The caller has to free()
 * the string returned. This function allocates memory, do not call on the
 * audio thread.
 *
 * @param synth
 * @return char*
 */
char* sfizz_get_unknown_opcodes(sfizz_synth_t* synth);

/**
 * @brief Check if the SFZ should be reloaded.
 *
 * Depending on the platform this can create file descriptors.
 *
 * @param synth
 * @return true if any included files (including the root file) have
 *              been modified since the sfz file was loaded.
 * @return false
 */
bool sfizz_should_reload_file(sfizz_synth_t* synth);

#ifdef __cplusplus
}
#endif
