// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SynthPrivate.h"
#include "StringViewHelpers.h"
#include <absl/strings/ascii.h>
#include <cstring>

namespace sfz {
static constexpr unsigned maxIndices = 8;

static bool extractMessage(const char* pattern, const char* path, unsigned* indices);
static uint64_t hashMessagePath(const char* path, const char* sig);

void sfz::Synth::dispatchMessage(Client& client, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    UNUSED(args);
    Impl& impl = *impl_;
    unsigned indices[maxIndices];

    switch (hashMessagePath(path, sig)) {
        #define MATCH(p, s) case hash(p "," s): \
            if (extractMessage(p, path, indices) && !strcmp(sig, s))

        #define GET_REGION_OR_BREAK(idx)            \
            if (idx >= impl.regions_.size())        \
                break;                              \
            const auto& region = *impl.regions_[idx];

        MATCH("/hello", "") {
            client.receive(delay, "/hello", "", nullptr);
        } break;

        MATCH("/region&/delay", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.delay);
        } break;

        MATCH("/region&/sample", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'s'>(delay, path, region.sampleId->filename().c_str());
        } break;

        MATCH("/region&/direction", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.sampleId->isReverse())
                client.receive<'s'>(delay, path, "reverse");
            else
                client.receive<'s'>(delay, path, "forward");
        } break;

        MATCH("/region&/delay_random", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.delayRandom);
        } break;

        MATCH("/region&/offset", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'h'>(delay, path, region.offset);
        } break;

        MATCH("/region&/offset_random", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'h'>(delay, path, region.offsetRandom);
        } break;

        MATCH("/region&/offset_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'h'>(delay, path, region.offsetCC.getWithDefault(indices[1]));
        } break;

        MATCH("/region&/end", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'h'>(delay, path, region.sampleEnd);
        } break;

        MATCH("/region&/enabled", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.disabled()) {
                client.receive<'F'>(delay, path, {});
            } else {
                client.receive<'T'>(delay, path, {});
            }
        } break;

        MATCH("/region&/count", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (!region.sampleCount) {
                client.receive<'N'>(delay, path, {});
            } else {
                client.receive<'h'>(delay, path, *region.sampleCount);
            }
        } break;

        MATCH("/region&/loop_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].h = region.loopRange.getStart();
            args[1].h = region.loopRange.getEnd();
            client.receive(delay, path, "hh", args);
        } break;

        MATCH("/region&/loop_mode", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (!region.loopMode) {
                client.receive<'s'>(delay, path, "no_loop");
                break;
            }

            switch (*region.loopMode) {
            case SfzLoopMode::no_loop:
                client.receive<'s'>(delay, path, "no_loop");
                break;
            case SfzLoopMode::loop_continuous:
                client.receive<'s'>(delay, path, "loop_continuous");
                break;
            case SfzLoopMode::loop_sustain:
                client.receive<'s'>(delay, path, "loop_sustain");
                break;
            case SfzLoopMode::one_shot:
                client.receive<'s'>(delay, path, "one_shot");
                break;
            }
        } break;

        MATCH("/region&/loop_crossfade", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.loopCrossfade);
        } break;

        MATCH("/region&/group", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'h'>(delay, path, region.group);
        } break;

        MATCH("/region&/off_by", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (!region.offBy) {
                client.receive<'N'>(delay, path, {});
            } else {
                client.receive<'h'>(delay, path, *region.offBy);
            }
        } break;

        MATCH("/region&/off_mode", "") {
            GET_REGION_OR_BREAK(indices[0])
            switch (region.offMode) {
            case SfzOffMode::time:
                client.receive<'s'>(delay, path, "time");
                break;
            case SfzOffMode::normal:
                client.receive<'s'>(delay, path, "normal");
                break;
            case SfzOffMode::fast:
                client.receive<'s'>(delay, path, "fast");
                break;
            }
        } break;

        MATCH("/region&/key_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].i = region.keyRange.getStart();
            args[1].i = region.keyRange.getEnd();
            client.receive(delay, path, "ii", args);
        } break;

        MATCH("/region&/off_time", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.offTime);
        } break;

        MATCH("/region&/pitch_keycenter", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.pitchKeycenter);
        } break;

        MATCH("/region&/vel_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].f = region.velocityRange.getStart();
            args[1].f = region.velocityRange.getEnd();
            client.receive(delay, path, "ff", args);
        } break;

        MATCH("/region&/bend_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].f = region.bendRange.getStart();
            args[1].f = region.bendRange.getEnd();
            client.receive(delay, path, "ff", args);
        } break;

        MATCH("/region&/cc_range&", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            const auto& conditions = region.ccConditions.getWithDefault(indices[1]);
            args[0].f = conditions.getStart();
            args[1].f = conditions.getEnd();
            client.receive(delay, path, "ff", args);
        } break;

        MATCH("/region&/sw_last", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.lastKeyswitch) {
                client.receive<'i'>(delay, path, *region.lastKeyswitch);
            } else if (region.lastKeyswitchRange) {
                sfizz_arg_t args[2];
                args[0].i = region.lastKeyswitchRange->getStart();
                args[1].i = region.lastKeyswitchRange->getEnd();
                client.receive(delay, path, "ii", args);
            } else {
                client.receive<'N'>(delay, path, {});
            }

        } break;

        MATCH("/region&/sw_label", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.keyswitchLabel) {
                client.receive<'s'>(delay, path, region.keyswitchLabel->c_str());
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/sw_up", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.upKeyswitch) {
                client.receive<'i'>(delay, path, *region.upKeyswitch);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/sw_down", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.downKeyswitch) {
                client.receive<'i'>(delay, path, *region.downKeyswitch);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/sw_previous", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.previousKeyswitch) {
                client.receive<'i'>(delay, path, *region.previousKeyswitch);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/sw_vel", "") {
            GET_REGION_OR_BREAK(indices[0])
            switch (region.velocityOverride) {
            case SfzVelocityOverride::current:
                client.receive<'s'>(delay, path, "current");
                break;
            case SfzVelocityOverride::previous:
                client.receive<'s'>(delay, path, "previous");
                break;
            }
        } break;

        MATCH("/region&/chanaft_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].i = region.aftertouchRange.getStart();
            args[1].i = region.aftertouchRange.getEnd();
            client.receive(delay, path, "ii", args);
        } break;

        MATCH("/region&/bpm_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].f = region.bpmRange.getStart();
            args[1].f = region.bpmRange.getEnd();
            client.receive(delay, path, "ff", args);
        } break;

        MATCH("/region&/rand_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].f = region.randRange.getStart();
            args[1].f = region.randRange.getEnd();
            client.receive(delay, path, "ff", args);
        } break;

        MATCH("/region&/seq_length", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'h'>(delay, path, region.sequenceLength);
        } break;

        MATCH("/region&/seq_position", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'h'>(delay, path, region.sequencePosition);
        } break;

        MATCH("/region&/trigger", "") {
            GET_REGION_OR_BREAK(indices[0])
            switch (region.trigger) {
            case SfzTrigger::attack:
                client.receive<'s'>(delay, path, "attack");
                break;
            case SfzTrigger::first:
                client.receive<'s'>(delay, path, "first");
                break;
            case SfzTrigger::release:
                client.receive<'s'>(delay, path, "release");
                break;
            case SfzTrigger::release_key:
                client.receive<'s'>(delay, path, "release_key");
                break;
            case SfzTrigger::legato:
                client.receive<'s'>(delay, path, "legato");
                break;
            }
        } break;

        MATCH("/region&/start_cc_range&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto trigger = region.ccTriggers.get(indices[1]);
            if (trigger) {
                sfizz_arg_t args[2];
                args[0].f = trigger->getStart();
                args[1].f = trigger->getEnd();
                client.receive(delay, path, "ff", args);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/volume", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.volume);
        } break;

        MATCH("/region&/volume_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto value = region.ccModDepth(indices[1], ModId::Volume);
            if (value) {
                client.receive<'f'>(delay, path, *value);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/volume_stepcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Volume);
            if (params) {
                client.receive<'f'>(delay, path, params->step);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/volume_smoothcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Volume);
            if (params) {
                client.receive<'i'>(delay, path, params->smooth);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/volume_curvecc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Volume);
            if (params) {
                client.receive<'i'>(delay, path, params->curve);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/pan", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.pan * 100.0f);
        } break;

        MATCH("/region&/pan_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto value = region.ccModDepth(indices[1], ModId::Pan);
            if (value) {
                client.receive<'f'>(delay, path, *value);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/pan_stepcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Pan);
            if (params) {
                client.receive<'f'>(delay, path, params->step);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/pan_smoothcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Pan);
            if (params) {
                client.receive<'i'>(delay, path, params->smooth);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/pan_curvecc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Pan);
            if (params) {
                client.receive<'i'>(delay, path, params->curve);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/width", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.width * 100.0f);
        } break;

        MATCH("/region&/width_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto value = region.ccModDepth(indices[1], ModId::Width);
            if (value) {
                client.receive<'f'>(delay, path, *value);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/width_stepcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Width);
            if (params) {
                client.receive<'f'>(delay, path, params->step);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/width_smoothcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Width);
            if (params) {
                client.receive<'i'>(delay, path, params->smooth);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/width_curvecc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Width);
            if (params) {
                client.receive<'i'>(delay, path, params->curve);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/position", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.position * 100.0f);
        } break;

        MATCH("/region&/position_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto value = region.ccModDepth(indices[1], ModId::Position);
            if (value) {
                client.receive<'f'>(delay, path, *value);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/position_stepcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Position);
            if (params) {
                client.receive<'f'>(delay, path, params->step);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/position_smoothcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Position);
            if (params) {
                client.receive<'i'>(delay, path, params->smooth);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/position_curvecc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Position);
            if (params) {
                client.receive<'i'>(delay, path, params->curve);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/amplitude", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.amplitude * 100.0f);
        } break;

        MATCH("/region&/amplitude_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto value = region.ccModDepth(indices[1], ModId::Amplitude);
            if (value) {
                client.receive<'f'>(delay, path, *value);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/amplitude_stepcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Amplitude);
            if (params) {
                client.receive<'f'>(delay, path, params->step);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/amplitude_smoothcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Amplitude);
            if (params) {
                client.receive<'i'>(delay, path, params->smooth);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/amplitude_curvecc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Amplitude);
            if (params) {
                client.receive<'i'>(delay, path, params->curve);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/amp_keycenter", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.ampKeycenter);
        } break;

        MATCH("/region&/amp_keytrack", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.ampKeytrack);
        } break;

        MATCH("/region&/amp_veltrack", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.ampVeltrack * 100.0f);
        } break;

        MATCH("/region&/amp_random", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.ampRandom);
        } break;

        MATCH("/region&/xfin_key_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].i = region.crossfadeKeyInRange.getStart();
            args[1].i = region.crossfadeKeyInRange.getEnd();
            client.receive(delay, path, "ii", args);
        } break;

        MATCH("/region&/xfout_key_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].i = region.crossfadeKeyOutRange.getStart();
            args[1].i = region.crossfadeKeyOutRange.getEnd();
            client.receive(delay, path, "ii", args);
        } break;

        MATCH("/region&/xfin_vel_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].f = region.crossfadeVelInRange.getStart();
            args[1].f = region.crossfadeVelInRange.getEnd();
            client.receive(delay, path, "ff", args);
        } break;

        MATCH("/region&/xfout_vel_range", "") {
            GET_REGION_OR_BREAK(indices[0])
            sfizz_arg_t args[2];
            args[0].f = region.crossfadeVelOutRange.getStart();
            args[1].f = region.crossfadeVelOutRange.getEnd();
            client.receive(delay, path, "ff", args);
        } break;

        MATCH("/region&/xfin_cc_range&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto range = region.crossfadeCCInRange.get(indices[1]);
            if (range) {
                sfizz_arg_t args[2];
                args[0].f = range->getStart();
                args[1].f = range->getEnd();
                client.receive(delay, path, "ff", args);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/xfout_cc_range&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto range = region.crossfadeCCOutRange.get(indices[1]);
            if (range) {
                sfizz_arg_t args[2];
                args[0].f = range->getStart();
                args[1].f = range->getEnd();
                client.receive(delay, path, "ff", args);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/xf_keycurve", "") {
            GET_REGION_OR_BREAK(indices[0])
            switch (region.crossfadeKeyCurve) {
            case SfzCrossfadeCurve::gain:
                client.receive<'s'>(delay, path, "gain");
                break;
            case SfzCrossfadeCurve::power:
                client.receive<'s'>(delay, path, "power");
                break;
            }
        } break;

        MATCH("/region&/xf_velcurve", "") {
            GET_REGION_OR_BREAK(indices[0])
            switch (region.crossfadeVelCurve) {
            case SfzCrossfadeCurve::gain:
                client.receive<'s'>(delay, path, "gain");
                break;
            case SfzCrossfadeCurve::power:
                client.receive<'s'>(delay, path, "power");
                break;
            }
        } break;

        MATCH("/region&/xf_cccurve", "") {
            GET_REGION_OR_BREAK(indices[0])
            switch (region.crossfadeCCCurve) {
            case SfzCrossfadeCurve::gain:
                client.receive<'s'>(delay, path, "gain");
                break;
            case SfzCrossfadeCurve::power:
                client.receive<'s'>(delay, path, "power");
                break;
            }
        } break;

        MATCH("/region&/global_volume", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.globalVolume);
        } break;

        MATCH("/region&/master_volume", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.masterVolume);
        } break;

        MATCH("/region&/group_volume", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.groupVolume);
        } break;

        MATCH("/region&/global_amplitude", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.globalAmplitude * 100.0f);
        } break;

        MATCH("/region&/master_amplitude", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.masterAmplitude * 100.0f);
        } break;

        MATCH("/region&/group_amplitude", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.groupAmplitude * 100.0f);
        } break;

        MATCH("/region&/pitch_keytrack", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.pitchKeytrack);
        } break;

        MATCH("/region&/pitch_veltrack", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.pitchVeltrack);
        } break;

        MATCH("/region&/pitch_random", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.pitchRandom);
        } break;

        MATCH("/region&/transpose", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.transpose);
        } break;

        MATCH("/region&/tune", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.tune);
        } break;

        MATCH("/region&/tune_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto value = region.ccModDepth(indices[1], ModId::Pitch);
            if (value) {
                client.receive<'f'>(delay, path, *value);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/tune_stepcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Pitch);
            if (params) {
                client.receive<'f'>(delay, path, params->step);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/tune_smoothcc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Pitch);
            if (params) {
                client.receive<'i'>(delay, path, params->smooth);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/tune_curvecc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto params = region.ccModParameters(indices[1], ModId::Pitch);
            if (params) {
                client.receive<'i'>(delay, path, params->curve);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/bend_up", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.bendUp);
        } break;

        MATCH("/region&/bend_down", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.bendDown);
        } break;

        MATCH("/region&/bend_step", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.bendStep);
        } break;

        MATCH("/region&/bend_smooth", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.bendSmooth);
        } break;

        MATCH("/region&/ampeg_attack", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.amplitudeEG.attack);
        } break;

        MATCH("/region&/ampeg_delay", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.amplitudeEG.delay);
        } break;

        MATCH("/region&/ampeg_decay", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.amplitudeEG.decay);
        } break;

        MATCH("/region&/ampeg_hold", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.amplitudeEG.hold);
        } break;

        MATCH("/region&/ampeg_release", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.amplitudeEG.release);
        } break;

        MATCH("/region&/ampeg_start", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.amplitudeEG.start);
        } break;

        MATCH("/region&/ampeg_sustain", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.amplitudeEG.sustain);
        } break;

        MATCH("/region&/ampeg_depth", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.amplitudeEG.depth);
        } break;

        MATCH("/region&/ampeg_vel&attack", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (indices[1] != 2)
                break;
            client.receive<'f'>(delay, path, region.amplitudeEG.vel2attack);
        } break;

        MATCH("/region&/ampeg_vel&delay", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (indices[1] != 2)
                break;
            client.receive<'f'>(delay, path, region.amplitudeEG.vel2delay);
        } break;

        MATCH("/region&/ampeg_vel&decay", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (indices[1] != 2)
                break;
            client.receive<'f'>(delay, path, region.amplitudeEG.vel2decay);
        } break;

        MATCH("/region&/ampeg_vel&hold", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (indices[1] != 2)
                break;
            client.receive<'f'>(delay, path, region.amplitudeEG.vel2hold);
        } break;

        MATCH("/region&/ampeg_vel&release", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (indices[1] != 2)
                break;
            client.receive<'f'>(delay, path, region.amplitudeEG.vel2release);
        } break;

        MATCH("/region&/ampeg_vel&sustain", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (indices[1] != 2)
                break;
            client.receive<'f'>(delay, path, region.amplitudeEG.vel2sustain);
        } break;

        MATCH("/region&/ampeg_vel&depth", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (indices[1] != 2)
                break;
            client.receive<'i'>(delay, path, region.amplitudeEG.vel2depth);
        } break;

        MATCH("/region&/note_polyphony", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.notePolyphony) {
                client.receive<'i'>(delay, path, *region.notePolyphony);
            } else {
                client.receive<'N'>(delay, path, {});
            }
        } break;

        MATCH("/region&/note_selfmask", "") {
            GET_REGION_OR_BREAK(indices[0])
            switch(region.selfMask) {
            case SfzSelfMask::mask:
                client.receive(delay, path, "T", nullptr);
                break;
            case SfzSelfMask::dontMask:
                client.receive(delay, path, "F", nullptr);
                break;
            }
        } break;

        MATCH("/region&/rt_dead", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.rtDead) {
                client.receive(delay, path, "T", nullptr);
            } else {
                client.receive(delay, path, "F", nullptr);
            }
        } break;

        MATCH("/region&/sustain_sw", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.checkSustain) {
                client.receive(delay, path, "T", nullptr);
            } else {
                client.receive(delay, path, "F", nullptr);
            }
        } break;

        MATCH("/region&/sostenuto_sw", "") {
            GET_REGION_OR_BREAK(indices[0])
            if (region.checkSostenuto) {
                client.receive(delay, path, "T", nullptr);
            } else {
                client.receive(delay, path, "F", nullptr);
            }
        } break;

        MATCH("/region&/sustain_cc", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'i'>(delay, path, region.sustainCC);
        } break;

        MATCH("/region&/sustain_lo", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.sustainThreshold);
        } break;

        MATCH("/region&/oscillator_phase", "") {
            GET_REGION_OR_BREAK(indices[0])
            client.receive<'f'>(delay, path, region.oscillatorPhase);
        } break;

        MATCH("/region&/effect&", "") {
            GET_REGION_OR_BREAK(indices[0])
            auto effectIdx = indices[1];
            if (indices[1] == 0)
                break;

            if (effectIdx < region.gainToEffect.size())
                client.receive<'f'>(delay, path, region.gainToEffect[effectIdx] * 100.0f);
        } break;

        MATCH("/region&/ampeg_attack_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            float value = region.amplitudeEG.ccAttack.getWithDefault(indices[1]);
            client.receive<'f'>(delay, path, value);
        } break;

        MATCH("/region&/ampeg_decay_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            float value = region.amplitudeEG.ccDecay.getWithDefault(indices[1]);
            client.receive<'f'>(delay, path, value);
        } break;

        MATCH("/region&/ampeg_delay_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            float value = region.amplitudeEG.ccDelay.getWithDefault(indices[1]);
            client.receive<'f'>(delay, path, value);
        } break;

        MATCH("/region&/ampeg_hold_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            float value = region.amplitudeEG.ccHold.getWithDefault(indices[1]);
            client.receive<'f'>(delay, path, value);
        } break;

        MATCH("/region&/ampeg_release_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            float value = region.amplitudeEG.ccRelease.getWithDefault(indices[1]);
            client.receive<'f'>(delay, path, value);
        } break;

        MATCH("/region&/ampeg_start_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            float value = region.amplitudeEG.ccStart.getWithDefault(indices[1]);
            client.receive<'f'>(delay, path, value);
        } break;

        MATCH("/region&/ampeg_sustain_cc&", "") {
            GET_REGION_OR_BREAK(indices[0])
            float value = region.amplitudeEG.ccSustain.getWithDefault(indices[1]);
            client.receive<'f'>(delay, path, value);
        } break;

        #define GET_FILTER_OR_BREAK(idx)                \
            if (idx >= region.filters.size())           \
                break;                                  \
            const auto& filter = region.filters[idx];

        MATCH("/region&/filter&/cutoff", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_FILTER_OR_BREAK(indices[1])
            client.receive<'f'>(delay, path, filter.cutoff);
        } break;

        MATCH("/region&/filter&/resonance", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_FILTER_OR_BREAK(indices[1])
            client.receive<'f'>(delay, path, filter.resonance);
        } break;

        MATCH("/region&/filter&/gain", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_FILTER_OR_BREAK(indices[1])
            client.receive<'f'>(delay, path, filter.gain);
        } break;

        MATCH("/region&/filter&/keycenter", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_FILTER_OR_BREAK(indices[1])
            client.receive<'i'>(delay, path, filter.keycenter);
        } break;

        MATCH("/region&/filter&/keytrack", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_FILTER_OR_BREAK(indices[1])
            client.receive<'i'>(delay, path, filter.keytrack);
        } break;

        MATCH("/region&/filter&/veltrack", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_FILTER_OR_BREAK(indices[1])
            client.receive<'i'>(delay, path, filter.veltrack);
        } break;

        MATCH("/region&/filter&/type", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_FILTER_OR_BREAK(indices[1])
            switch (filter.type) {
            case FilterType::kFilterLpf1p: client.receive<'s'>(delay, path, "lpf_1p"); break;
            case FilterType::kFilterHpf1p: client.receive<'s'>(delay, path, "hpf_1p"); break;
            case FilterType::kFilterLpf2p: client.receive<'s'>(delay, path, "lpf_2p"); break;
            case FilterType::kFilterHpf2p: client.receive<'s'>(delay, path, "hpf_2p"); break;
            case FilterType::kFilterBpf2p: client.receive<'s'>(delay, path, "bpf_2p"); break;
            case FilterType::kFilterBrf2p: client.receive<'s'>(delay, path, "brf_2p"); break;
            case FilterType::kFilterBpf1p: client.receive<'s'>(delay, path, "bpf_1p"); break;
            case FilterType::kFilterBrf1p: client.receive<'s'>(delay, path, "brf_1p"); break;
            case FilterType::kFilterApf1p: client.receive<'s'>(delay, path, "apf_1p"); break;
            case FilterType::kFilterLpf2pSv: client.receive<'s'>(delay, path, "lpf_2p_sv"); break;
            case FilterType::kFilterHpf2pSv: client.receive<'s'>(delay, path, "hpf_2p_sv"); break;
            case FilterType::kFilterBpf2pSv: client.receive<'s'>(delay, path, "bpf_2p_sv"); break;
            case FilterType::kFilterBrf2pSv: client.receive<'s'>(delay, path, "brf_2p_sv"); break;
            case FilterType::kFilterLpf4p: client.receive<'s'>(delay, path, "lpf_4p"); break;
            case FilterType::kFilterHpf4p: client.receive<'s'>(delay, path, "hpf_4p"); break;
            case FilterType::kFilterLpf6p: client.receive<'s'>(delay, path, "lpf_6p"); break;
            case FilterType::kFilterHpf6p: client.receive<'s'>(delay, path, "hpf_6p"); break;
            case FilterType::kFilterPink: client.receive<'s'>(delay, path, "pink"); break;
            case FilterType::kFilterLsh: client.receive<'s'>(delay, path, "lsh"); break;
            case FilterType::kFilterHsh: client.receive<'s'>(delay, path, "hsh"); break;
            case FilterType::kFilterPeq: client.receive<'s'>(delay, path, "peq"); break;
            case FilterType::kFilterBpf4p: client.receive<'s'>(delay, path, "bpf_4p"); break;
            case FilterType::kFilterBpf6p: client.receive<'s'>(delay, path, "bpf_6p"); break;
            case FilterType::kFilterNone: client.receive<'s'>(delay, path, "none"); break;
            }
        } break;

        #undef GET_FILTER_OR_BREAK

        #define GET_EQ_OR_BREAK(idx)                    \
            if (idx >= region.equalizers.size())           \
                break;                                  \
            const auto& eq = region.equalizers[idx];

        MATCH("/region&/eq&/gain", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_EQ_OR_BREAK(indices[1])
            client.receive<'f'>(delay, path, eq.gain);
        } break;

        MATCH("/region&/eq&/bandwidth", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_EQ_OR_BREAK(indices[1])
            client.receive<'f'>(delay, path, eq.bandwidth);
        } break;

        MATCH("/region&/eq&/frequency", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_EQ_OR_BREAK(indices[1])
            client.receive<'f'>(delay, path, eq.frequency);
        } break;

        MATCH("/region&/eq&/vel&freq", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_EQ_OR_BREAK(indices[1])
            if (indices[2] != 2)
                break;
            client.receive<'f'>(delay, path, eq.vel2frequency);
        } break;

        MATCH("/region&/eq&/vel&gain", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_EQ_OR_BREAK(indices[1])
            if (indices[2] != 2)
                break;
            client.receive<'f'>(delay, path, eq.vel2gain);
        } break;

        MATCH("/region&/eq&/type", "") {
            GET_REGION_OR_BREAK(indices[0])
            GET_EQ_OR_BREAK(indices[1])
            switch (eq.type) {
            case EqType::kEqNone: client.receive<'s'>(delay, path, "none"); break;
            case EqType::kEqPeak: client.receive<'s'>(delay, path, "peak"); break;
            case EqType::kEqLowShelf: client.receive<'s'>(delay, path, "lshelf"); break;
            case EqType::kEqHighShelf: client.receive<'s'>(delay, path, "hshelf"); break;
            }
        } break;

        #undef GET_EQ_OR_BREAK

        #undef GET_REGION_OR_BREAK
        #undef MATCH
        // TODO...
    }
}

static bool extractMessage(const char* pattern, const char* path, unsigned* indices)
{
    unsigned nthIndex = 0;

    while (const char *endp = strchr(pattern, '&')) {
        if (nthIndex == maxIndices)
            return false;

        size_t length = endp - pattern;
        if (strncmp(pattern, path, length))
            return false;
        pattern += length;
        path += length;

        length = 0;
        while (absl::ascii_isdigit(path[length]))
            ++length;

        if (!absl::SimpleAtoi(absl::string_view(path, length), &indices[nthIndex++]))
            return false;

        pattern += 1;
        path += length;
    }

    return !strcmp(path, pattern);
}

static uint64_t hashMessagePath(const char* path, const char* sig)
{
    uint64_t h = Fnv1aBasis;
    while (unsigned char c = *path++) {
        if (!absl::ascii_isdigit(c))
            h = hashByte(c, h);
        else {
            h = hashByte('&', h);
            while (absl::ascii_isdigit(*path))
                ++path;
        }
    }
    h = hashByte(',', h);
    while (unsigned char c = *sig++)
        h = hashByte(c, h);
    return h;
}

} // namespace sfz
