// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include "sfizz_lv2.h"
#include <spin_mutex.h>
#include <sfizz.h>
#include <stdbool.h>
#include <stdint.h>
#include <absl/types/optional.h>
#include <atomic>
#include <mutex>

#define DEFAULT_SCALA_FILE  "Contents/Resources/DefaultScale.scl"
#define DEFAULT_SFZ_FILE    "Contents/Resources/DefaultInstrument.sfz"
// This assumes that the longest path is the default sfz file; if not, change it
#define MAX_BUNDLE_PATH_SIZE (MAX_PATH_SIZE - sizeof(DEFAULT_SFZ_FILE))

struct sfizz_plugin_t
{
    // Features
    LV2_URID_Map *map {};
    LV2_URID_Unmap *unmap {};
    LV2_Worker_Schedule *worker {};
    LV2_Log_Log *log {};
    LV2_Midnam *midnam {};

    // Ports
    const LV2_Atom_Sequence *control_port {};
    LV2_Atom_Sequence *notify_port {};
    LV2_Atom_Sequence *automate_port {};
    float *output_buffers[2] {};
    const float *volume_port {};
    const float *polyphony_port {};
    const float *oversampling_port {};
    const float *preload_port {};
    const float *freewheel_port {};
    const float *scala_root_key_port {};
    const float *tuning_frequency_port {};
    const float *stretch_tuning_port {};
    const float *sample_quality_port {};
    const float *oscillator_quality_port {};
    float *active_voices_port {};
    float *num_curves_port {};
    float *num_masters_port {};
    float *num_groups_port {};
    float *num_regions_port {};
    float *num_samples_port {};

    // Atom forge
    LV2_Atom_Forge forge_notify {};       ///< Forge for writing notification atoms in run thread
    LV2_Atom_Forge forge_automate {};     ///< Forge for writing automation atoms in run thread
    LV2_Atom_Forge forge_secondary {};    ///< Forge for writing into other buffers

    // Logger
    LV2_Log_Logger logger {};

    // URIs
    LV2_URID midi_event_uri {};
    LV2_URID options_interface_uri {};
    LV2_URID max_block_length_uri {};
    LV2_URID nominal_block_length_uri {};
    LV2_URID sample_rate_uri {};
    LV2_URID atom_object_uri {};
    LV2_URID atom_blank_uri {};
    LV2_URID atom_float_uri {};
    LV2_URID atom_double_uri {};
    LV2_URID atom_int_uri {};
    LV2_URID atom_long_uri {};
    LV2_URID atom_urid_uri {};
    LV2_URID atom_path_uri {};
    LV2_URID patch_set_uri {};
    LV2_URID patch_get_uri {};
    LV2_URID patch_put_uri {};
    LV2_URID patch_property_uri {};
    LV2_URID patch_value_uri {};
    LV2_URID patch_body_uri {};
    LV2_URID state_changed_uri {};
    LV2_URID sfizz_sfz_file_uri {};
    LV2_URID sfizz_scala_file_uri {};
    LV2_URID sfizz_num_voices_uri {};
    LV2_URID sfizz_preload_size_uri {};
    LV2_URID sfizz_oversampling_uri {};
    LV2_URID sfizz_log_status_uri {};
    LV2_URID sfizz_check_modification_uri {};
    LV2_URID sfizz_active_voices_uri {};
    LV2_URID sfizz_osc_blob_uri {};
    LV2_URID time_position_uri {};
    LV2_URID time_bar_uri {};
    LV2_URID time_bar_beat_uri {};
    LV2_URID time_beat_unit_uri {};
    LV2_URID time_beats_per_bar_uri {};
    LV2_URID time_beats_per_minute_uri {};
    LV2_URID time_speed_uri {};

    // CC parameters
    sfizz_lv2_ccmap* ccmap {};

    // Sfizz related data
    sfizz_synth_t *synth {};
    sfizz_client_t *client {};
    spin_mutex_t *synth_mutex {};
    bool expect_nominal_block_length {};
    char sfz_file_path[MAX_PATH_SIZE] {};
    char scala_file_path[MAX_PATH_SIZE] {};
    int num_voices {};
    unsigned int preload_size {};
    sfizz_oversampling_factor_t oversampling {};
    float stretch_tuning {};
    volatile bool check_modification {};
    int max_block_size {};
    int sample_counter {};
    float sample_rate {};
    std::atomic<int> must_update_midnam {};
    volatile bool must_automate_cc {};

    // Current instrument description
    std::mutex *sfz_blob_mutex {};
    volatile int sfz_blob_serial {};
    const uint8_t *volatile sfz_blob_data {};
    volatile uint32_t sfz_blob_size {};

    // Current CC values in the synth (synchronized by `synth_mutex`)
    // updated by hdcc or file load
    float *cc_current {};

    // CC queued for automation on next run(). (synchronized by `synth_mutex`)
    absl::optional<float>* ccauto {};
    volatile bool have_ccauto {};

    // Timing data
    int bar {};
    double bar_beat {};
    int beats_per_bar {};
    int beat_unit {};
    double bpm_tempo {};
    double speed {};

    // Paths
    char bundle_path[MAX_BUNDLE_PATH_SIZE] {};

    // OSC
    uint8_t osc_temp[OSC_TEMP_SIZE] {};
};
