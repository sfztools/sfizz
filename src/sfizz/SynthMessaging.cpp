// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Defaults.h"
#include "Region.h"
#include "SynthMessagingHelper.hpp"

namespace sfz {

void sfz::Synth::dispatchMessage(Client& client, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    Impl& impl = *impl_;
    MessagingHelper m {client, delay, path, sig, args, impl};
    using ModParam = MessagingHelper::ModParam;

    switch (hashMessagePath(path, sig)) {
        #define MATCH(p, s) case hash(p "," s): if (m.match(p, s))
        MATCH("/hello", "") { m.reply(""); } break;
        //----------------------------------------------------------------------
        MATCH("/num_regions", "") { m.reply(impl.layers_.size()); } break;
        MATCH("/num_groups", "") { m.reply(impl.numGroups_); } break;
        MATCH("/num_masters", "") { m.reply(impl.numMasters_); } break;
        MATCH("/num_curves", "") { m.reply(impl.resources_.getCurves().getNumCurves()); } break;
        MATCH("/num_samples", "") { m.reply(impl.resources_.getFilePool().getNumPreloadedSamples()); } break;
        MATCH("/octave_offset", "") { m.reply(impl.octaveOffset_); } break;
        MATCH("/note_offset", "") { m.reply(impl.noteOffset_); } break;
        MATCH("/num_outputs", "") { m.reply(impl.numOutputs_); } break;
        MATCH("/num_active_voices", "") { m.reply(uint32_t(impl.voiceManager_.getNumActiveVoices())); } break;
        MATCH("/sustain_cancels_release", "") { m.reply(&SynthConfig::sustainCancelsRelease); } break;
        MATCH("/sample_quality", "") { m.reply(&SynthConfig::liveSampleQuality); } break;
        MATCH("/sustain_cancels_release", "s") { m.set(&SynthConfig::sustainCancelsRelease, Default::sustainCancelsRelease); } break;
        MATCH("/sustain_cancels_release", "T") { m.set(&SynthConfig::sustainCancelsRelease, Default::sustainCancelsRelease); } break;
        MATCH("/sustain_cancels_release", "F") { m.set(&SynthConfig::sustainCancelsRelease, Default::sustainCancelsRelease); } break;
        MATCH("/sample_quality", "i") { m.set(&SynthConfig::liveSampleQuality, Default::sampleQuality); } break;
        MATCH("/oscillator_quality", "") { m.reply(&SynthConfig::liveOscillatorQuality); } break;
        MATCH("/oscillator_quality", "i") { m.set(&SynthConfig::liveOscillatorQuality, Default::oscillatorQuality); } break;
        MATCH("/freewheeling_sample_quality", "") { m.reply(&SynthConfig::freeWheelingSampleQuality); } break;
        MATCH("/freewheeling_sample_quality", "i") { m.set(&SynthConfig::freeWheelingSampleQuality, Default::sampleQuality); } break;
        MATCH("/freewheeling_oscillator_quality", "") { m.reply(&SynthConfig::freeWheelingOscillatorQuality); } break;
        MATCH("/freewheeling_oscillator_quality", "i") { m.set(&SynthConfig::freeWheelingOscillatorQuality, Default::oscillatorQuality); } break;
        //----------------------------------------------------------------------
        MATCH("/key/slots", "") { m.reply(impl.keySlots_); } break;
        MATCH("/key&/label", "") { if (auto k = m.sindex(0)) m.reply(impl.getKeyLabel(*k)); } break;
        //----------------------------------------------------------------------
        MATCH("/root_path", "") { m.reply(impl.rootPath_); } break;
        MATCH("/image", "") { m.reply(impl.image_.c_str()); } break;
        MATCH("/image_controls", "") { m.reply(impl.image_controls_.c_str()); } break;
        //----------------------------------------------------------------------
        MATCH("/sw/last/slots", "") { m.reply(impl.swLastSlots_); } break;
        MATCH("/sw/last/current", "") { m.reply(impl.currentSwitch_); } break;
        MATCH("/sw/last/&/label", "") { if (auto k = m.sindex(0)) m.reply(impl.getKeyswitchLabel(*k)); } break;
        //----------------------------------------------------------------------
        MATCH("/cc/slots", "") { m.reply(impl.currentUsedCCs_); } break;
        MATCH("/cc&/default", "") { if (auto cc = m.checkCC()) m.reply(impl.defaultCCValues_[*cc]); } break;
        MATCH("/cc&/value", "") { if (auto cc = m.checkCC()) m.reply(impl.resources_.getMidiState().getCCValue(*cc)); } break;
        MATCH("/cc&/value", "f") { if (auto cc = m.checkCC()) impl.resources_.getMidiState().ccEvent(delay, *cc, args[0].f); } break;
        MATCH("/cc&/label", "") { if (auto cc = m.checkCC()) m.reply(impl.getCCLabel(*cc)); } break;
        MATCH("/cc/changed", "") { m.reply(impl.changedCCsThisCycle_); } break;
        MATCH("/cc/changed~", "") {  m.reply(impl.changedCCsLastCycle_); } break;
        MATCH("/sustain_or_sostenuto/slots", "") { m.reply(impl.sustainOrSostenuto_); } break;
        MATCH("/aftertouch", "") { m.reply(impl.resources_.getMidiState().getChannelAftertouch()); } break;
        MATCH("/poly_aftertouch/&", "") { if (auto note = m.checkNote()) m.reply(impl.resources_.getMidiState().getPolyAftertouch(*note)); } break;
        MATCH("/pitch_bend", "") { m.reply(impl.resources_.getMidiState().getPitchBend()); } break;
        //----------------------------------------------------------------------
        MATCH("/mem/buffers", "") { m.reply(BufferCounter::counter().getTotalBytes()); } break;
        //----------------------------------------------------------------------
        MATCH("/region&/delay", "") { m.reply(&Region::delay); } break;
        MATCH("/region&/delay", "f") { m.set(&Region::delay, Default::delay); } break;
        MATCH("/region&/delay_random", "") { m.reply(&Region::delayRandom); } break;
        MATCH("/region&/delay_random", "f") { m.set(&Region::delayRandom, Default::delayRandom); } break;
        MATCH("/region&/sample", "") { if (auto region = m.getRegion()) { m.reply(region->sampleId->filename()); } } break;
        MATCH("/region&/direction", "") { if (auto region = m.getRegion()) { m.reply(region->sampleId->isReverse() ? "reverse" : "forward"); } } break;
        MATCH("/region&/delay_cc&", "") { m.reply(&Region::delayCC); } break;
        MATCH("/region&/delay_cc&", "f") { m.set(&Region::delayCC, Default::delayMod); } break;
        MATCH("/region&/offset", "") { m.reply(&Region::offset); } break;
        MATCH("/region&/offset", "h") { m.set(&Region::offset, Default::offset); } break;
        MATCH("/region&/offset_random", "") { m.reply(&Region::offsetRandom); } break;
        MATCH("/region&/offset_random", "h") { m.set(&Region::offsetRandom, Default::offsetRandom); } break;
        MATCH("/region&/offset_cc&", "") { m.reply(&Region::offsetCC); } break;
        MATCH("/region&/offset_cc&", "h") { m.set(&Region::offsetCC, Default::offsetMod); } break;
        MATCH("/region&/end", "") { m.reply(&Region::sampleEnd); } break;
        MATCH("/region&/end", "h") { m.set(&Region::sampleEnd, Default::sampleEnd); } break;
        MATCH("/region&/end_cc&", "") { m.reply(&Region::endCC); } break;
        MATCH("/region&/end_cc&", "h") { m.set(&Region::endCC, Default::sampleEndMod); } break;
        MATCH("/region&/enabled", "") { if (auto region = m.getRegion()) { m.reply(!region->disabled()); } } break;
        MATCH("/region&/trigger_on_note", "") { m.reply(&Region::triggerOnNote); } break;
        MATCH("/region&/trigger_on_cc", "") { m.reply(&Region::triggerOnCC); } break;
        MATCH("/region&/use_timer_range", "") { m.reply(&Region::useTimerRange); } break;
        MATCH("/region&/count", "") { m.reply(&Region::sampleCount); } break;
        MATCH("/region&/count", "i") { m.set(&Region::sampleCount, Default::sampleCount); } break;
        MATCH("/region&/count", "N") { m.set(&Region::sampleCount, Default::sampleCount); } break;
        MATCH("/region&/loop_range", "") { m.reply(&Region::loopRange); } break;
        MATCH("/region&/loop_range", "hh") { m.set(&Region::loopRange, Default::loopStart, Default::loopEnd); } break;
        MATCH("/region&/loop_start_cc&", "") { m.reply(&Region::loopStartCC); } break;
        MATCH("/region&/loop_start_cc&", "h") { m.set(&Region::loopStartCC, Default::loopStart); } break;
        MATCH("/region&/loop_end_cc&", "") { m.reply(&Region::loopEndCC); } break;
        MATCH("/region&/loop_end_cc&", "h") { m.set(&Region::loopEndCC, Default::loopEnd); } break;
        MATCH("/region&/loop_mode", "") { m.reply(&Region::loopMode, LoopMode::no_loop); } break;
        MATCH("/region&/loop_mode", "s") { m.set(&Region::loopMode, Default::loopMode); } break;
        MATCH("/region&/loop_crossfade", "") { m.reply(&Region::loopCrossfade); } break;
        MATCH("/region&/loop_crossfade", "f") { m.set(&Region::loopCrossfade, Default::loopCrossfade); } break;
        MATCH("/region&/loop_count", "") { m.reply(&Region::loopCount); } break;
        MATCH("/region&/loop_count", "i") { m.set(&Region::loopCount, Default::loopCount); } break;
        MATCH("/region&/loop_count", "N") { m.set(&Region::loopCount, Default::loopCount); } break;
        MATCH("/region&/output", "") { m.reply(&Region::output); } break;
        MATCH("/region&/output", "i") { m.set(&Region::output, Default::output); } break;
        MATCH("/region&/group", "") { m.reply(&Region::group); } break;
        MATCH("/region&/group", "h") { m.set(&Region::group, Default::group); } break;
        MATCH("/region&/off_by", "") { m.reply(&Region::offBy); } break;
        MATCH("/region&/off_by", "h") { m.set(&Region::offBy, Default::group); } break;
        MATCH("/region&/off_by", "N") { m.set(&Region::offBy, Default::group); } break;
        MATCH("/region&/off_mode", "") { m.reply(&Region::offMode); } break;
        MATCH("/region&/off_mode", "s") { m.set(&Region::offMode, Default::offMode); } break;
        MATCH("/region&/key_range", "") { m.reply(&Region::keyRange); } break;
        MATCH("/region&/key_range", "ii") { m.set(&Region::keyRange, Default::loKey, Default::hiKey); } break;
        MATCH("/region&/off_time", "") { m.reply(&Region::offTime); } break;
        MATCH("/region&/off_time", "f") { m.set(&Region::offTime, Default::offTime); } break;
        MATCH("/region&/pitch_keycenter", "") { m.reply(&Region::pitchKeycenter); } break;
        MATCH("/region&/pitch_keycenter", "i") { m.set(&Region::pitchKeycenter, Default::key); } break;
        MATCH("/region&/vel_range", "") { m.reply(&Region::velocityRange); } break;
        MATCH("/region&/vel_range", "ff") { m.set(&Region::velocityRange); } break;
        MATCH("/region&/bend_range", "") { m.reply(&Region::bendRange); } break;
        MATCH("/region&/bend_range", "ff") { m.set(&Region::bendRange); } break;
        MATCH("/region&/program_range", "") { m.reply(&Region::programRange); } break;
        MATCH("/region&/program_range", "ii") { m.set(&Region::programRange); } break;
        MATCH("/region&/cc_range&", "") { m.reply(&Region::ccConditions); } break;
        MATCH("/region&/cc_range&", "ff") { m.set(&Region::ccConditions); } break;
        MATCH("/region&/sw_last", "") {
            if (auto region = m.getRegion()) {
                if (region->lastKeyswitch) m.reply(region->lastKeyswitch);
                else m.reply(region->lastKeyswitchRange);
            }
        } break;
        MATCH("/region&/sw_last", "i") {
            if (auto region = m.getRegion()) {
                m.set(&Region::lastKeyswitch, Default::key);
                region->lastKeyswitchRange.reset();
            }
        } break;
        MATCH("/region&/sw_last", "ii") {
            if (auto region = m.getRegion()) {
                region->lastKeyswitch.reset();
                region->lastKeyswitchRange.emplace(args[0].i, args[1].i);
            }
        } break;
        MATCH("/region&/sw_label", "") { m.reply(&Region::keyswitchLabel); } break;
        MATCH("/region&/sw_label", "s") { m.set(&Region::keyswitchLabel); } break;
        MATCH("/region&/sw_up", "") { m.reply(&Region::upKeyswitch); } break;
        MATCH("/region&/sw_up", "i") { m.set(&Region::upKeyswitch, Default::key); } break;
        MATCH("/region&/sw_up", "s") { m.set(&Region::upKeyswitch, Default::key); } break;
        MATCH("/region&/sw_down", "") { m.reply(&Region::downKeyswitch); } break;
        MATCH("/region&/sw_down", "i") { m.set(&Region::downKeyswitch, Default::key); } break;
        MATCH("/region&/sw_down", "s") { m.set(&Region::downKeyswitch, Default::key); } break;
        MATCH("/region&/sw_previous", "") { m.reply(&Region::previousKeyswitch); } break;
        MATCH("/region&/sw_previous", "i") { m.set(&Region::previousKeyswitch, Default::key); } break;
        MATCH("/region&/sw_previous", "s") { m.set(&Region::previousKeyswitch, Default::key); } break;
        MATCH("/region&/sw_vel", "") { m.reply(&Region::velocityOverride); } break;
        MATCH("/region&/sw_vel", "s") { m.set(&Region::velocityOverride, Default::velocityOverride); } break;
        MATCH("/region&/chanaft_range", "") { m.reply(&Region::aftertouchRange); } break;
        MATCH("/region&/chanaft_range", "ff") { m.set(&Region::aftertouchRange); } break;
        MATCH("/region&/polyaft_range", "") { m.reply(&Region::polyAftertouchRange); } break;
        MATCH("/region&/polyaft_range", "ff") { m.set(&Region::polyAftertouchRange); } break;
        MATCH("/region&/bpm_range", "") { m.reply(&Region::bpmRange); } break;
        MATCH("/region&/bpm_range", "ff") { m.set(&Region::bpmRange, Default::loBPM, Default::hiBPM); } break;
        MATCH("/region&/rand_range", "") { m.reply(&Region::randRange); } break;
        MATCH("/region&/rand_range", "ff") { m.set(&Region::randRange, Default::loNormalized, Default::hiNormalized); } break;
        MATCH("/region&/seq_length", "") { m.reply(&Region::sequenceLength); } break;
        MATCH("/region&/seq_length", "i") { m.set(&Region::sequenceLength, Default::sequence); } break;
        MATCH("/region&/seq_position", "") { m.reply(&Region::sequencePosition); } break;
        MATCH("/region&/seq_position", "i") { m.set(&Region::sequencePosition, Default::sequence); } break;
        MATCH("/region&/trigger", "") { m.reply(&Region::trigger); } break;
        MATCH("/region&/trigger", "s") { m.set(&Region::trigger, Default::trigger); } break;
        MATCH("/region&/start_cc_range&", "") { m.reply(&Region::ccTriggers, false); } break;
        MATCH("/region&/start_cc_range&", "ff") { m.set(&Region::ccTriggers); } break;
        MATCH("/region&/volume", "") { m.reply(&Region::volume); } break;
        MATCH("/region&/volume", "f") { m.set(&Region::volume, Default::volume); } break;
        // Probably need to rethink the way we set these in both the Region parsing and here before making changes
        MATCH("/region&/volume_cc&", "") { m.reply(ModId::Volume, ModParam::Depth); } break;
        MATCH("/region&/volume_stepcc&", "") { m.reply(ModId::Volume, ModParam::Step); } break;
        MATCH("/region&/volume_smoothcc&", "") { m.reply(ModId::Volume, ModParam::Smooth); } break;
        MATCH("/region&/volume_curvecc&", "") { m.reply(ModId::Volume, ModParam::Curve); } break;
        MATCH("/region&/pan", "") { m.reply(&Region::pan, Default::pan); } break;
        MATCH("/region&/pan", "f") { m.set(&Region::pan, Default::pan); } break;
        MATCH("/region&/pan_cc&", "") { m.reply(ModId::Pan, ModParam::Depth, Default::pan); } break;
        MATCH("/region&/pan_stepcc&", "") { m.reply(ModId::Pan, ModParam::Step, Default::pan); } break;
        MATCH("/region&/pan_smoothcc&", "") { m.reply(ModId::Pan, ModParam::Smooth, Default::pan); } break;
        MATCH("/region&/pan_curvecc&", "") { m.reply(ModId::Pan, ModParam::Curve, Default::pan); } break;
        MATCH("/region&/width", "") { m.reply(&Region::width, Default::width); } break;
        MATCH("/region&/width", "f") { m.set(&Region::width, Default::width); } break;
        MATCH("/region&/width_cc&", "") { m.reply(ModId::Width, ModParam::Depth, Default::width); } break;
        MATCH("/region&/width_stepcc&", "") { m.reply(ModId::Width, ModParam::Step, Default::width); } break;
        MATCH("/region&/width_smoothcc&", "") { m.reply(ModId::Width, ModParam::Smooth, Default::width); } break;
        MATCH("/region&/width_curvecc&", "") { m.reply(ModId::Width, ModParam::Curve, Default::width); } break;
        MATCH("/region&/timer_range", "") { m.reply(&Region::timerRange); } break;
        MATCH("/region&/position", "") { m.reply(&Region::position, Default::position); } break;
        MATCH("/region&/position", "f") { m.set(&Region::position, Default::position); } break;
        MATCH("/region&/position_cc&", "") { m.reply(ModId::Position, ModParam::Depth, Default::position); } break;
        MATCH("/region&/position_stepcc&", "") { m.reply(ModId::Position, ModParam::Step, Default::position); } break;
        MATCH("/region&/position_smoothcc&", "") { m.reply(ModId::Position, ModParam::Smooth, Default::position); } break;
        MATCH("/region&/position_curvecc&", "") { m.reply(ModId::Position, ModParam::Curve, Default::position); } break;
        MATCH("/region&/amplitude", "") { m.reply(&Region::amplitude, Default::amplitude); } break;
        MATCH("/region&/amplitude", "f") { m.set(&Region::amplitude, Default::amplitude); } break;
        MATCH("/region&/amplitude_cc&", "") { m.reply(ModId::Amplitude, ModParam::Depth, Default::amplitude); } break;
        MATCH("/region&/amplitude_stepcc&", "") { m.reply(ModId::Amplitude, ModParam::Step, Default::amplitude); } break;
        MATCH("/region&/amplitude_smoothcc&", "") { m.reply(ModId::Amplitude, ModParam::Smooth, Default::amplitude); } break;
        MATCH("/region&/amplitude_curvecc&", "") { m.reply(ModId::Amplitude, ModParam::Curve, Default::amplitude); } break;
        MATCH("/region&/amp_keycenter", "") { m.reply(&Region::ampKeycenter); } break;
        MATCH("/region&/amp_keycenter", "i") { m.set(&Region::ampKeycenter, Default::key); } break;
        MATCH("/region&/amp_keytrack", "") { m.reply(&Region::ampKeytrack); } break;
        MATCH("/region&/amp_keytrack", "f") { m.set(&Region::ampKeytrack, Default::ampKeytrack); } break;
        MATCH("/region&/amp_veltrack", "") { m.reply(&Region::ampVeltrack, Default::ampVeltrack); } break;
        MATCH("/region&/amp_veltrack", "f") { m.set(&Region::ampVeltrack, Default::ampVeltrack); } break;
        MATCH("/region&/amp_veltrack_cc&", "") { m.reply(&Region::ampVeltrackCC, false, ModParam::Depth, Default::ampVeltrackMod); } break;
        MATCH("/region&/amp_veltrack_cc&", "f") { m.set(&Region::ampVeltrackCC, ModParam::Depth, Default::ampVeltrackMod); } break;
        MATCH("/region&/amp_veltrack_curvecc&", "") { m.reply(&Region::ampVeltrackCC, false, ModParam::Curve, Default::ampVeltrackMod); } break;
        MATCH("/region&/amp_veltrack_curvecc&", "i") { m.set(&Region::ampVeltrackCC, ModParam::Curve, Default::ampVeltrackMod); } break;
        MATCH("/region&/amp_random", "") { m.reply(&Region::ampRandom); } break;
        MATCH("/region&/amp_random", "f") { m.set(&Region::ampRandom, Default::ampRandom); } break;
        MATCH("/region&/xfin_key_range", "") { m.reply(&Region::crossfadeKeyInRange); } break;
        MATCH("/region&/xfin_key_range", "ii") { m.set(&Region::crossfadeKeyInRange, Default::loKey, Default::hiKey); } break;
        MATCH("/region&/xfout_key_range", "") { m.reply(&Region::crossfadeKeyOutRange); } break;
        MATCH("/region&/xfout_key_range", "ii") { m.set(&Region::crossfadeKeyOutRange, Default::loKey, Default::hiKey); } break;
        MATCH("/region&/xfin_vel_range", "") { m.reply(&Region::crossfadeVelInRange); } break;
        MATCH("/region&/xfin_vel_range", "ff") { m.set(&Region::crossfadeVelInRange); } break;
        MATCH("/region&/xfout_vel_range", "") { m.reply(&Region::crossfadeVelOutRange); } break;
        MATCH("/region&/xfout_vel_range", "ff") { m.set(&Region::crossfadeVelOutRange); } break;
        MATCH("/region&/xfin_cc_range&", "") { m.reply(&Region::crossfadeCCInRange, false); } break;
        MATCH("/region&/xfin_cc_range&", "ff") { m.set(&Region::crossfadeCCInRange); } break;
        MATCH("/region&/xfout_cc_range&", "") { m.reply(&Region::crossfadeCCOutRange, false); } break;
        MATCH("/region&/xfout_cc_range&", "ff") { m.set(&Region::crossfadeCCOutRange); } break;
        MATCH("/region&/xf_keycurve", "") { m.reply(&Region::crossfadeKeyCurve); } break;
        MATCH("/region&/xf_keycurve", "s") { m.set(&Region::crossfadeKeyCurve, Default::crossfadeCurve); } break;
        MATCH("/region&/xf_velcurve", "") { m.reply(&Region::crossfadeVelCurve); } break;
        MATCH("/region&/xf_velcurve", "s") { m.set(&Region::crossfadeVelCurve, Default::crossfadeCurve); } break;
        MATCH("/region&/xf_cccurve", "") { m.reply(&Region::crossfadeCCCurve); } break;
        MATCH("/region&/xf_cccurve", "s") { m.set(&Region::crossfadeCCCurve, Default::crossfadeCurve); } break;
        MATCH("/region&/global_volume", "") { m.reply(&Region::globalVolume); } break;
        MATCH("/region&/global_volume", "f") { m.set(&Region::globalVolume, Default::volume); } break;
        MATCH("/region&/master_volume", "") { m.reply(&Region::masterVolume); } break;
        MATCH("/region&/master_volume", "f") { m.set(&Region::masterVolume, Default::volume); } break;
        MATCH("/region&/group_volume", "") { m.reply(&Region::groupVolume); } break;
        MATCH("/region&/group_volume", "f") { m.set(&Region::groupVolume, Default::volume); } break;
        MATCH("/region&/global_amplitude", "") { m.reply(&Region::globalAmplitude, Default::amplitude); } break;
        MATCH("/region&/global_amplitude", "f") { m.set(&Region::globalAmplitude, Default::amplitude); } break;
        MATCH("/region&/master_amplitude", "") { m.reply(&Region::masterAmplitude, Default::amplitude); } break;
        MATCH("/region&/master_amplitude", "f") { m.set(&Region::masterAmplitude, Default::amplitude); } break;
        MATCH("/region&/group_amplitude", "") { m.reply(&Region::groupAmplitude, Default::amplitude); } break;
        MATCH("/region&/group_amplitude", "f") { m.set(&Region::groupAmplitude, Default::amplitude); } break;
        MATCH("/region&/pitch_keytrack", "") { m.reply(&Region::pitchKeytrack); } break;
        MATCH("/region&/pitch_keytrack", "f") { m.set(&Region::pitchKeytrack, Default::pitchKeytrack); } break;
        MATCH("/region&/pitch_veltrack", "") { m.reply(&Region::pitchVeltrack); } break;
        MATCH("/region&/pitch_veltrack", "f") { m.set(&Region::pitchVeltrack, Default::pitchVeltrack); } break;
        MATCH("/region&/pitch_veltrack_cc&", "") { m.reply(&Region::pitchVeltrackCC, false, ModParam::Depth); } break;
        MATCH("/region&/pitch_veltrack_cc&", "f") { m.set(&Region::pitchVeltrackCC, ModParam::Depth, Default::pitchVeltrackMod); } break;
        MATCH("/region&/pitch_veltrack_curvecc&", "") { m.reply(&Region::pitchVeltrackCC, false, ModParam::Curve); } break;
        MATCH("/region&/pitch_veltrack_curvecc&", "i") { m.set(&Region::pitchVeltrackCC, ModParam::Curve, Default::pitchVeltrackMod); } break;
        MATCH("/region&/pitch_random", "") { m.reply(&Region::pitchRandom); } break;
        MATCH("/region&/pitch_random", "f") { m.set(&Region::pitchRandom, Default::pitchRandom); } break;
        MATCH("/region&/transpose", "") { m.reply(&Region::transpose); } break;
        MATCH("/region&/transpose", "f") { m.set(&Region::transpose, Default::transpose); } break;
        MATCH("/region&/pitch", "") { m.reply(&Region::pitch); } break;
        MATCH("/region&/pitch", "f") { m.set(&Region::pitch, Default::pitch); } break;
        MATCH("/region&/pitch_cc&", "") { m.reply(ModId::Pitch, ModParam::Depth, Default::pitch); } break;
        MATCH("/region&/pitch_stepcc&", "") { m.reply(ModId::Pitch, ModParam::Step, Default::pitch); } break;
        MATCH("/region&/pitch_smoothcc&", "") { m.reply(ModId::Pitch, ModParam::Smooth, Default::pitch); } break;
        MATCH("/region&/pitch_curvecc&", "") { m.reply(ModId::Pitch, ModParam::Curve, Default::pitch); } break;
        MATCH("/region&/bend_up", "") { m.reply(&Region::bendUp); } break;
        MATCH("/region&/bend_up", "f") { m.set(&Region::bendUp, Default::bendUp); } break;
        MATCH("/region&/bend_down", "") { m.reply(&Region::bendDown); } break;
        MATCH("/region&/bend_down", "f") { m.set(&Region::bendDown, Default::bendDown); } break;
        MATCH("/region&/bend_step", "") { m.reply(&Region::bendStep); } break;
        MATCH("/region&/bend_step", "f") { m.set(&Region::bendStep, Default::bendStep); } break;
        MATCH("/region&/bend_smooth", "") { m.reply(&Region::bendSmooth); } break;
        MATCH("/region&/bend_smooth", "i") { m.set(&Region::bendSmooth, Default::smoothCC); } break;
        MATCH("/region&/ampeg_attack", "") { m.reply(&Region::amplitudeEG, &EGDescription::attack); } break;
        MATCH("/region&/ampeg_delay", "") { m.reply(&Region::amplitudeEG, &EGDescription::delay); } break;
        MATCH("/region&/ampeg_decay", "") { m.reply(&Region::amplitudeEG, &EGDescription::decay); } break;
        MATCH("/region&/ampeg_hold", "") { m.reply(&Region::amplitudeEG, &EGDescription::hold); } break;
        MATCH("/region&/ampeg_release", "") { m.reply(&Region::amplitudeEG, &EGDescription::release); } break;
        MATCH("/region&/ampeg_start", "") { m.reply(&Region::amplitudeEG, &EGDescription::start, Default::egPercent); } break;
        MATCH("/region&/ampeg_sustain", "") { m.reply(&Region::amplitudeEG, &EGDescription::sustain, Default::egPercent); } break;
        MATCH("/region&/ampeg_depth", "") { m.reply(&Region::amplitudeEG, &EGDescription::depth); } break;
        MATCH("/region&/ampeg_attack_cc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccAttack, ModParam::Depth); } break;
        MATCH("/region&/ampeg_attack_curvecc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccAttack, ModParam::Curve); } break;
        MATCH("/region&/ampeg_decay_cc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccDecay, ModParam::Depth); } break;
        MATCH("/region&/ampeg_decay_curvecc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccDecay, ModParam::Curve); } break;
        MATCH("/region&/ampeg_delay_cc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccDelay, ModParam::Depth); } break;
        MATCH("/region&/ampeg_delay_curvecc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccDelay, ModParam::Curve); } break;
        MATCH("/region&/ampeg_hold_cc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccHold, ModParam::Depth); } break;
        MATCH("/region&/ampeg_hold_curvecc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccHold, ModParam::Curve); } break;
        MATCH("/region&/ampeg_release_cc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccRelease, ModParam::Depth); } break;
        MATCH("/region&/ampeg_release_curvecc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccRelease, ModParam::Curve); } break;
        MATCH("/region&/ampeg_sustain_cc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccSustain, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_sustain_curvecc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccSustain, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_start_cc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccStart, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_start_curvecc&", "") { m.reply(&Region::amplitudeEG, &EGDescription::ccStart, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_vel&attack", "") { m.reply(&Region::amplitudeEG, &EGDescription::vel2attack); } break;
        MATCH("/region&/ampeg_vel&delay", "") { m.reply(&Region::amplitudeEG, &EGDescription::vel2delay); } break;
        MATCH("/region&/ampeg_vel&decay", "") { m.reply(&Region::amplitudeEG, &EGDescription::vel2decay); } break;
        MATCH("/region&/ampeg_vel&hold", "") { m.reply(&Region::amplitudeEG, &EGDescription::vel2hold); } break;
        MATCH("/region&/ampeg_vel&release", "") { m.reply(&Region::amplitudeEG, &EGDescription::vel2release); } break;
        MATCH("/region&/ampeg_vel&sustain", "") { m.reply(&Region::amplitudeEG, &EGDescription::vel2sustain, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_vel&depth", "") { m.reply(&Region::amplitudeEG, &EGDescription::vel2depth); } break;
        MATCH("/region&/ampeg_dynamic", "") { m.reply(&Region::amplitudeEG, &EGDescription::dynamic); } break;
        MATCH("/region&/ampeg_attack", "f") { m.set(&Region::amplitudeEG, &EGDescription::attack, Default::egTime); } break;
        MATCH("/region&/ampeg_delay", "f") { m.set(&Region::amplitudeEG, &EGDescription::delay, Default::egTime); } break;
        MATCH("/region&/ampeg_decay", "f") { m.set(&Region::amplitudeEG, &EGDescription::decay, Default::egTime); } break;
        MATCH("/region&/ampeg_hold", "f") { m.set(&Region::amplitudeEG, &EGDescription::hold, Default::egTime); } break;
        MATCH("/region&/ampeg_release", "f") { m.set(&Region::amplitudeEG, &EGDescription::release, Default::egTime); } break;
        MATCH("/region&/ampeg_start", "f") { m.set(&Region::amplitudeEG, &EGDescription::start, Default::egPercent); } break;
        MATCH("/region&/ampeg_sustain", "f") { m.set(&Region::amplitudeEG, &EGDescription::sustain, Default::egPercent); } break;
        MATCH("/region&/ampeg_depth", "f") { m.set(&Region::amplitudeEG, &EGDescription::depth, Default::egDepth); } break;
        MATCH("/region&/ampeg_attack_cc&", "f") { m.set(&Region::amplitudeEG, &EGDescription::ccAttack, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_attack_curvecc&", "i") { m.set(&Region::amplitudeEG, &EGDescription::ccAttack, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_decay_cc&", "f") { m.set(&Region::amplitudeEG, &EGDescription::ccDecay, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_decay_curvecc&", "i") { m.set(&Region::amplitudeEG, &EGDescription::ccDecay, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_delay_cc&", "f") { m.set(&Region::amplitudeEG, &EGDescription::ccDelay, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_delay_curvecc&", "i") { m.set(&Region::amplitudeEG, &EGDescription::ccDelay, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_hold_cc&", "f") { m.set(&Region::amplitudeEG, &EGDescription::ccHold, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_hold_curvecc&", "i") { m.set(&Region::amplitudeEG, &EGDescription::ccHold, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_release_cc&", "f") { m.set(&Region::amplitudeEG, &EGDescription::ccRelease, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_release_curvecc&", "i") { m.set(&Region::amplitudeEG, &EGDescription::ccRelease, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_sustain_cc&", "f") { m.set(&Region::amplitudeEG, &EGDescription::ccSustain, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_sustain_curvecc&", "i") { m.set(&Region::amplitudeEG, &EGDescription::ccSustain, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_start_cc&", "f") { m.set(&Region::amplitudeEG, &EGDescription::ccStart, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_start_curvecc&", "i") { m.set(&Region::amplitudeEG, &EGDescription::ccStart, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_vel&attack", "f") { m.set(&Region::amplitudeEG, &EGDescription::vel2attack, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_vel&delay", "f") { m.set(&Region::amplitudeEG, &EGDescription::vel2delay, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_vel&decay", "f") { m.set(&Region::amplitudeEG, &EGDescription::vel2decay, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_vel&hold", "f") { m.set(&Region::amplitudeEG, &EGDescription::vel2hold, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_vel&release", "f") { m.set(&Region::amplitudeEG, &EGDescription::vel2release, Default::egTimeMod); } break;
        MATCH("/region&/ampeg_vel&sustain", "f") { m.set(&Region::amplitudeEG, &EGDescription::vel2sustain, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_vel&depth", "f") { m.set(&Region::amplitudeEG, &EGDescription::vel2depth, Default::egDepth); } break;
        MATCH("/region&/ampeg_dynamic", "T") { m.set(&Region::amplitudeEG, &EGDescription::dynamic, Default::egDynamic);  } break;
        MATCH("/region&/ampeg_dynamic", "F") { m.set(&Region::amplitudeEG, &EGDescription::dynamic, Default::egDynamic); } break;
        MATCH("/region&/ampeg_dynamic", "s") { m.set(&Region::amplitudeEG, &EGDescription::dynamic, Default::egDynamic); } break;
        MATCH("/region&/fileg_attack", "") { m.reply(&Region::filterEG, &EGDescription::attack); } break;
        MATCH("/region&/fileg_delay", "") { m.reply(&Region::filterEG, &EGDescription::delay); } break;
        MATCH("/region&/fileg_decay", "") { m.reply(&Region::filterEG, &EGDescription::decay); } break;
        MATCH("/region&/fileg_hold", "") { m.reply(&Region::filterEG, &EGDescription::hold); } break;
        MATCH("/region&/fileg_release", "") { m.reply(&Region::filterEG, &EGDescription::release); } break;
        MATCH("/region&/fileg_start", "") { m.reply(&Region::filterEG, &EGDescription::start, Default::egPercentMod); } break;
        MATCH("/region&/fileg_sustain", "") { m.reply(&Region::filterEG, &EGDescription::sustain, Default::egPercentMod); } break;
        MATCH("/region&/fileg_depth", "") { m.reply(&Region::filterEG, &EGDescription::depth); } break;
        MATCH("/region&/fileg_attack_cc&", "") { m.reply(&Region::filterEG, &EGDescription::ccAttack, ModParam::Depth); } break;
        MATCH("/region&/fileg_attack_curvecc&", "") { m.reply(&Region::filterEG, &EGDescription::ccAttack, ModParam::Curve); } break;
        MATCH("/region&/fileg_decay_cc&", "") { m.reply(&Region::filterEG, &EGDescription::ccDecay, ModParam::Depth); } break;
        MATCH("/region&/fileg_decay_curvecc&", "") { m.reply(&Region::filterEG, &EGDescription::ccDecay, ModParam::Curve); } break;
        MATCH("/region&/fileg_delay_cc&", "") { m.reply(&Region::filterEG, &EGDescription::ccDelay, ModParam::Depth); } break;
        MATCH("/region&/fileg_delay_curvecc&", "") { m.reply(&Region::filterEG, &EGDescription::ccDelay, ModParam::Curve); } break;
        MATCH("/region&/fileg_hold_cc&", "") { m.reply(&Region::filterEG, &EGDescription::ccHold, ModParam::Depth); } break;
        MATCH("/region&/fileg_hold_curvecc&", "") { m.reply(&Region::filterEG, &EGDescription::ccHold, ModParam::Curve); } break;
        MATCH("/region&/fileg_release_cc&", "") { m.reply(&Region::filterEG, &EGDescription::ccRelease, ModParam::Depth); } break;
        MATCH("/region&/fileg_release_curvecc&", "") { m.reply(&Region::filterEG, &EGDescription::ccRelease, ModParam::Curve); } break;
        MATCH("/region&/fileg_sustain_cc&", "") { m.reply(&Region::filterEG, &EGDescription::ccSustain, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/fileg_sustain_curvecc&", "") { m.reply(&Region::filterEG, &EGDescription::ccSustain, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/fileg_start_cc&", "") { m.reply(&Region::filterEG, &EGDescription::ccStart, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/fileg_start_curvecc&", "") { m.reply(&Region::filterEG, &EGDescription::ccStart, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/fileg_dynamic", "") { m.reply(&Region::filterEG, &EGDescription::dynamic); } break;
        MATCH("/region&/fileg_attack", "f") { m.set(&Region::filterEG, &EGDescription::attack, Default::egTime); } break;
        MATCH("/region&/fileg_delay", "f") { m.set(&Region::filterEG, &EGDescription::delay, Default::egTime); } break;
        MATCH("/region&/fileg_decay", "f") { m.set(&Region::filterEG, &EGDescription::decay, Default::egTime); } break;
        MATCH("/region&/fileg_hold", "f") { m.set(&Region::filterEG, &EGDescription::hold, Default::egTime); } break;
        MATCH("/region&/fileg_release", "f") { m.set(&Region::filterEG, &EGDescription::release, Default::egTime); } break;
        MATCH("/region&/fileg_start", "f") { m.set(&Region::filterEG, &EGDescription::start, Default::egPercent); } break;
        MATCH("/region&/fileg_sustain", "f") { m.set(&Region::filterEG, &EGDescription::sustain, Default::egPercent); } break;
        MATCH("/region&/fileg_depth", "f") { m.set(&Region::filterEG, &EGDescription::depth, Default::egDepth); } break;
        MATCH("/region&/fileg_attack_cc&", "f") { m.set(&Region::filterEG, &EGDescription::ccAttack, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/fileg_attack_curvecc&", "i") { m.set(&Region::filterEG, &EGDescription::ccAttack, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/fileg_decay_cc&", "f") { m.set(&Region::filterEG, &EGDescription::ccDecay, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/fileg_decay_curvecc&", "i") { m.set(&Region::filterEG, &EGDescription::ccDecay, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/fileg_delay_cc&", "f") { m.set(&Region::filterEG, &EGDescription::ccDelay, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/fileg_delay_curvecc&", "i") { m.set(&Region::filterEG, &EGDescription::ccDelay, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/fileg_hold_cc&", "f") { m.set(&Region::filterEG, &EGDescription::ccHold, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/fileg_hold_curvecc&", "i") { m.set(&Region::filterEG, &EGDescription::ccHold, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/fileg_release_cc&", "f") { m.set(&Region::filterEG, &EGDescription::ccRelease, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/fileg_release_curvecc&", "i") { m.set(&Region::filterEG, &EGDescription::ccRelease, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/fileg_sustain_cc&", "f") { m.set(&Region::filterEG, &EGDescription::ccSustain, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/fileg_sustain_curvecc&", "i") { m.set(&Region::filterEG, &EGDescription::ccSustain, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/fileg_start_cc&", "f") { m.set(&Region::filterEG, &EGDescription::ccStart, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/fileg_start_curvecc&", "i") { m.set(&Region::filterEG, &EGDescription::ccStart, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/fileg_dynamic", "T") {  m.set(&Region::filterEG, &EGDescription::dynamic, Default::egDynamic); } break;
        MATCH("/region&/fileg_dynamic", "F") {  m.set(&Region::filterEG, &EGDescription::dynamic, Default::egDynamic); } break;
        MATCH("/region&/fileg_dynamic", "s") { m.set(&Region::filterEG, &EGDescription::dynamic, Default::egDynamic); } break;
        MATCH("/region&/pitcheg_attack", "") { m.reply(&Region::pitchEG, &EGDescription::attack); } break;
        MATCH("/region&/pitcheg_delay", "") { m.reply(&Region::pitchEG, &EGDescription::delay); } break;
        MATCH("/region&/pitcheg_decay", "") { m.reply(&Region::pitchEG, &EGDescription::decay); } break;
        MATCH("/region&/pitcheg_hold", "") { m.reply(&Region::pitchEG, &EGDescription::hold); } break;
        MATCH("/region&/pitcheg_release", "") { m.reply(&Region::pitchEG, &EGDescription::release); } break;
        MATCH("/region&/pitcheg_start", "") { m.reply(&Region::pitchEG, &EGDescription::start, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_sustain", "") { m.reply(&Region::pitchEG, &EGDescription::sustain, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_depth", "") { m.reply(&Region::pitchEG, &EGDescription::depth); } break;
        MATCH("/region&/pitcheg_attack_cc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccAttack, ModParam::Depth); } break;
        MATCH("/region&/pitcheg_attack_curvecc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccAttack, ModParam::Curve); } break;
        MATCH("/region&/pitcheg_decay_cc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccDecay, ModParam::Depth); } break;
        MATCH("/region&/pitcheg_decay_curvecc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccDecay, ModParam::Curve); } break;
        MATCH("/region&/pitcheg_delay_cc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccDelay, ModParam::Depth); } break;
        MATCH("/region&/pitcheg_delay_curvecc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccDelay, ModParam::Curve); } break;
        MATCH("/region&/pitcheg_hold_cc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccHold, ModParam::Depth); } break;
        MATCH("/region&/pitcheg_hold_curvecc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccHold, ModParam::Curve); } break;
        MATCH("/region&/pitcheg_release_cc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccRelease, ModParam::Depth); } break;
        MATCH("/region&/pitcheg_release_curvecc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccRelease, ModParam::Curve); } break;
        MATCH("/region&/pitcheg_sustain_cc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccSustain, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_sustain_curvecc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccSustain, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_start_cc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccStart, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_start_curvecc&", "") { m.reply(&Region::pitchEG, &EGDescription::ccStart, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_dynamic", "") { m.reply(&Region::pitchEG, &EGDescription::dynamic); } break;
        MATCH("/region&/pitcheg_attack", "f") { m.set(&Region::pitchEG, &EGDescription::attack, Default::egTime); } break;
        MATCH("/region&/pitcheg_delay", "f") { m.set(&Region::pitchEG, &EGDescription::delay, Default::egTime); } break;
        MATCH("/region&/pitcheg_decay", "f") { m.set(&Region::pitchEG, &EGDescription::decay, Default::egTime); } break;
        MATCH("/region&/pitcheg_hold", "f") { m.set(&Region::pitchEG, &EGDescription::hold, Default::egTime); } break;
        MATCH("/region&/pitcheg_release", "f") { m.set(&Region::pitchEG, &EGDescription::release, Default::egTime); } break;
        MATCH("/region&/pitcheg_start", "f") { m.set(&Region::pitchEG, &EGDescription::start, Default::egPercent); } break;
        MATCH("/region&/pitcheg_sustain", "f") { m.set(&Region::pitchEG, &EGDescription::sustain, Default::egPercent); } break;
        MATCH("/region&/pitcheg_depth", "f") { m.set(&Region::pitchEG, &EGDescription::depth, Default::egDepth); } break;
        MATCH("/region&/pitcheg_attack_cc&", "f") { m.set(&Region::pitchEG, &EGDescription::ccAttack, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_attack_curvecc&", "i") { m.set(&Region::pitchEG, &EGDescription::ccAttack, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_decay_cc&", "f") { m.set(&Region::pitchEG, &EGDescription::ccDecay, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_decay_curvecc&", "i") { m.set(&Region::pitchEG, &EGDescription::ccDecay, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_delay_cc&", "f") { m.set(&Region::pitchEG, &EGDescription::ccDelay, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_delay_curvecc&", "i") { m.set(&Region::pitchEG, &EGDescription::ccDelay, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_hold_cc&", "f") { m.set(&Region::pitchEG, &EGDescription::ccHold, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_hold_curvecc&", "i") { m.set(&Region::pitchEG, &EGDescription::ccHold, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_release_cc&", "f") { m.set(&Region::pitchEG, &EGDescription::ccRelease, ModParam::Depth, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_release_curvecc&", "i") { m.set(&Region::pitchEG, &EGDescription::ccRelease, ModParam::Curve, Default::egTimeMod); } break;
        MATCH("/region&/pitcheg_sustain_cc&", "f") { m.set(&Region::pitchEG, &EGDescription::ccSustain, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_sustain_curvecc&", "i") { m.set(&Region::pitchEG, &EGDescription::ccSustain, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_start_cc&", "f") { m.set(&Region::pitchEG, &EGDescription::ccStart, ModParam::Depth, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_start_curvecc&", "i") { m.set(&Region::pitchEG, &EGDescription::ccStart, ModParam::Curve, Default::egPercentMod); } break;
        MATCH("/region&/pitcheg_dynamic", "T") { if (auto region = m.getRegion()) { if (region->pitchEG) { region->pitchEG->dynamic = true;} }  } break;
        MATCH("/region&/pitcheg_dynamic", "F") { if (auto region = m.getRegion()) { if (region->pitchEG) { region->pitchEG->dynamic = false;} } } break;
        MATCH("/region&/pitcheg_dynamic", "s") { m.set(&Region::pitchEG, &EGDescription::dynamic, Default::egDynamic); } break;
        MATCH("/region&/note_polyphony", "") { m.reply(&Region::notePolyphony); } break;
        MATCH("/region&/note_polyphony", "i") { m.set(&Region::notePolyphony, Default::notePolyphony); } break;
        MATCH("/region&/rt_dead", "") { m.reply(&Region::rtDead); } break;
        MATCH("/region&/rt_dead", "s") { m.set(&Region::rtDead, Default::rtDead); } break;
        MATCH("/region&/rt_dead", "T") { m.set(&Region::rtDead, Default::rtDead); } break;
        MATCH("/region&/rt_dead", "F") { m.set(&Region::rtDead, Default::rtDead); } break;
        MATCH("/region&/sustain_sw", "") { m.reply(&Region::checkSustain); } break;
        MATCH("/region&/sustain_sw", "s") { m.set(&Region::checkSustain, Default::checkSustain); } break;
        MATCH("/region&/sustain_sw", "T") { m.set(&Region::checkSustain, Default::checkSustain); } break;
        MATCH("/region&/sustain_sw", "F") { m.set(&Region::checkSustain, Default::checkSustain); } break;
        MATCH("/region&/sostenuto_sw", "") { m.reply(&Region::checkSostenuto); } break;
        MATCH("/region&/sostenuto_sw", "s") { m.set(&Region::checkSostenuto, Default::checkSostenuto); } break;
        MATCH("/region&/sostenuto_sw", "T") { m.set(&Region::checkSostenuto, Default::checkSostenuto); } break;
        MATCH("/region&/sostenuto_sw", "F") { m.set(&Region::checkSostenuto, Default::checkSostenuto); } break;
        MATCH("/region&/sustain_cc", "") { m.reply(&Region::sustainCC); } break;
        MATCH("/region&/sustain_cc", "i") { m.set(&Region::sustainCC, Default::sustainCC); } break;
        MATCH("/region&/sostenuto_cc", "") { m.reply(&Region::sostenutoCC); } break;
        MATCH("/region&/sostenuto_cc", "i") { m.set(&Region::sostenutoCC, Default::sostenutoCC); } break;
        MATCH("/region&/sustain_lo", "") { m.reply(&Region::sustainThreshold); } break;
        MATCH("/region&/sustain_lo", "f") { m.set(&Region::sustainThreshold); } break;
        MATCH("/region&/sostenuto_lo", "") { m.reply(&Region::sostenutoThreshold); } break;
        MATCH("/region&/sostenuto_lo", "f") { m.set(&Region::sostenutoThreshold); } break;
        MATCH("/region&/note_selfmask", "") { m.reply(&Region::selfMask); } break;
        MATCH("/region&/note_selfmask", "s") { m.set(&Region::selfMask, Default::selfMask); } break;
        MATCH("/region&/oscillator_phase", "") { m.reply(&Region::oscillatorPhase); } break;
        MATCH("/region&/oscillator_phase", "f") { m.set(&Region::oscillatorPhase, Default::oscillatorPhase); } break;
        MATCH("/region&/oscillator_quality", "") { m.reply(&Region::oscillatorQuality); } break;
        MATCH("/region&/oscillator_quality", "i") { m.set(&Region::oscillatorQuality, Default::oscillatorQuality); } break;
        MATCH("/region&/oscillator_mode", "") { m.reply(&Region::oscillatorMode); } break;
        MATCH("/region&/oscillator_mode", "i") { m.set(&Region::oscillatorMode, Default::oscillatorMode); } break;
        MATCH("/region&/oscillator_multi", "") { m.reply(&Region::oscillatorMulti); } break;
        MATCH("/region&/oscillator_multi", "i") { m.set(&Region::oscillatorMulti, Default::oscillatorMulti); } break;
        MATCH("/region&/oscillator_detune", "") { m.reply(&Region::oscillatorDetune); } break;
        MATCH("/region&/oscillator_detune", "f") { m.set(&Region::oscillatorDetune, Default::oscillatorDetune); } break;
        MATCH("/region&/oscillator_mod_depth", "") { m.reply(&Region::oscillatorModDepth, Default::oscillatorModDepth); } break;
        MATCH("/region&/oscillator_mod_depth", "f") { m.set(&Region::oscillatorModDepth, Default::oscillatorModDepth); } break;
        // TODO: detune cc, mod depth cc

        MATCH("/region&/effect&", "") {
            if (auto region = m.getRegion())
                if (auto effectIdx = m.sindex(1))
                    if (effectIdx > 0 && effectIdx < ssize(region->gainToEffect))
                        m.reply(region->gainToEffect[*effectIdx], Default::effect);
        } break;
        MATCH("/region&/effect&", "f") {
            if (auto region = m.getRegion())
                if (auto effectIdx = m.sindex(1))
                    if (effectIdx > 0 && effectIdx < ssize(region->gainToEffect))
                        m.set(region->gainToEffect[*effectIdx], Default::effect);
        } break;
        MATCH("/region&/add_filter", "") {
            if (auto region = m.getRegion()) {
                region->filters.emplace_back();
                int32_t index = region->filters.size() - 1;
                impl.settingsPerVoice_.maxFilters = max(impl.settingsPerVoice_.maxFilters, region->flexEGs.size());
                impl.applySettingsPerVoice();
                m.reply(index);
            }
        } break;
        MATCH("/region&/filter&/cutoff", "") { m.reply(&FilterDescription::cutoff); } break;
        MATCH("/region&/filter&/cutoff", "f") { m.set(&FilterDescription::cutoff, Default::filterCutoff); } break;
        MATCH("/region&/filter&/cutoff_cc&", "") { m.reply(ModId::FilCutoff, ModParam::Depth); } break;
        MATCH("/region&/filter&/cutoff_curvecc&", "") { m.reply(ModId::FilCutoff, ModParam::Curve); } break;
        MATCH("/region&/filter&/cutoff_stepcc&", "") { m.reply(ModId::FilCutoff, ModParam::Step); } break;
        MATCH("/region&/filter&/cutoff_smoothcc&", "") { m.reply(ModId::FilCutoff, ModParam::Smooth); } break;
        MATCH("/region&/filter&/resonance", "") { m.reply(&FilterDescription::resonance); } break;
        MATCH("/region&/filter&/resonance", "f") { m.set(&FilterDescription::resonance, Default::filterResonance); } break;
        MATCH("/region&/filter&/gain", "") { m.reply(&FilterDescription::gain); } break;
        MATCH("/region&/filter&/gain", "f") { m.set(&FilterDescription::gain, Default::filterGain); } break;
        MATCH("/region&/filter&/keycenter", "") { m.reply(&FilterDescription::keycenter); } break;
        MATCH("/region&/filter&/keycenter", "i") { m.set(&FilterDescription::keycenter, Default::key); } break;
        MATCH("/region&/filter&/keytrack", "") { m.reply(&FilterDescription::keytrack); } break;
        MATCH("/region&/filter&/keytrack", "f") { m.set(&FilterDescription::keytrack, Default::filterKeytrack); } break;
        MATCH("/region&/filter&/veltrack", "") { m.reply(&FilterDescription::veltrack); } break;
        MATCH("/region&/filter&/veltrack", "f") { m.set(&FilterDescription::veltrack, Default::filterVeltrack); } break;
        MATCH("/region&/filter&/veltrack_cc&", "") { m.reply(&FilterDescription::veltrackCC, ModParam::Depth); } break;
        MATCH("/region&/filter&/veltrack_cc&", "f") { m.set(&FilterDescription::veltrackCC, ModParam::Depth, Default::filterVeltrackMod); } break;
        MATCH("/region&/filter&/veltrack_curvecc&", "") { m.reply(&FilterDescription::veltrackCC, ModParam::Curve); } break;
        MATCH("/region&/filter&/veltrack_curvecc&", "i") { m.set(&FilterDescription::veltrackCC, ModParam::Curve, Default::filterVeltrackMod); } break;
        MATCH("/region&/filter&/type", "") { m.reply(&FilterDescription::type); } break;
        MATCH("/region&/filter&/type", "s") { m.set(&FilterDescription::type, Default::filter); } break;
        //----------------------------------------------------------------------
        MATCH("/region&/add_eq", "") {
            if (auto region = m.getRegion()) {
                region->equalizers.emplace_back();
                int32_t index = region->equalizers.size() - 1;
                impl.settingsPerVoice_.maxEQs = max(impl.settingsPerVoice_.maxEQs, region->flexEGs.size());
                impl.applySettingsPerVoice();
                m.reply(index);
            }
        } break;
        MATCH("/region&/eq&/gain", "") { m.reply(&EQDescription::gain); } break;
        MATCH("/region&/eq&/gain", "f") { m.set(&EQDescription::gain, Default::eqGain); } break;
        MATCH("/region&/eq&/bandwidth", "") { m.reply(&EQDescription::bandwidth); } break;
        MATCH("/region&/eq&/bandwidth", "f") { m.set(&EQDescription::bandwidth, Default::eqBandwidth); } break;
        MATCH("/region&/eq&/frequency", "") { m.reply(&EQDescription::frequency); } break;
        MATCH("/region&/eq&/frequency", "f") { m.set(&EQDescription::frequency, Default::eqFrequency); } break;
        MATCH("/region&/eq&/vel&freq", "") { m.reply(&EQDescription::vel2frequency); } break;
        MATCH("/region&/eq&/vel&freq", "f") { m.set(&EQDescription::vel2frequency, Default::eqVel2Frequency); } break;
        MATCH("/region&/eq&/vel&gain", "") { m.reply(&EQDescription::vel2gain); } break;
        MATCH("/region&/eq&/vel&gain", "f") { m.set(&EQDescription::vel2gain, Default::eqVel2Gain); } break;
        MATCH("/region&/eq&/type", "") { m.reply(&EQDescription::type); } break;
        MATCH("/region&/eq&/type", "s") { m.set(&EQDescription::type, Default::eq); } break;
        //----------------------------------------------------------------------
        MATCH("/region&/lfo&/wave", "") { m.reply(&LFODescription::Sub::wave); } break;
        MATCH("/region&/lfo&/wave", "i") { m.set(&LFODescription::Sub::wave, Default::lfoWave); } break;
        MATCH("/region&/lfo&/wave&", "") { m.reply(&LFODescription::Sub::wave); } break;
        MATCH("/region&/lfo&/wave&", "i") { m.set(&LFODescription::Sub::wave, Default::lfoWave); } break;
        //----------------------------------------------------------------------
        MATCH("/region&/add_eg", "") {
            if (auto region = m.getRegion()) {
                region->flexEGs.emplace_back();
                region->flexEGs.back().points.emplace_back();
                int32_t index = region->flexEGs.size() - 1;
                impl.settingsPerVoice_.maxFlexEGs = max(impl.settingsPerVoice_.maxFlexEGs, region->flexEGs.size());
                impl.applySettingsPerVoice();
                m.reply(index);
            }
        } break;
        MATCH("/region&/eg&/add_point", "") {
            if (auto region = m.getRegion()) {
                if (auto eg = m.getEG(*region)) {
                    eg->points.emplace_back();
                    int32_t index = eg->points.size() - 2;
                    m.reply(index);
                }
            }
        } break;
        MATCH("/region&/eg&/point&/time", "") { m.reply(&FlexEGPoint::time); } break;
        MATCH("/region&/eg&/point&/time", "f") { m.set(&FlexEGPoint::time, Default::flexEGPointTime); } break;
        MATCH("/region&/eg&/point&/time_cc&", "") { m.reply(&FlexEGPoint::ccTime); } break;
        MATCH("/region&/eg&/point&/time_cc&", "f") { m.set(&FlexEGPoint::ccTime, Default::flexEGPointTimeMod); } break;
        MATCH("/region&/eg&/point&/level", "") { m.reply(&FlexEGPoint::level); } break;
        MATCH("/region&/eg&/point&/level", "f") { m.set(&FlexEGPoint::level, Default::flexEGPointLevel); } break;
        MATCH("/region&/eg&/point&/level_cc&", "") { m.reply(&FlexEGPoint::ccLevel); } break;
        MATCH("/region&/eg&/point&/level_cc&", "f") { m.set(&FlexEGPoint::ccLevel, Default::flexEGPointLevelMod); } break;
        //----------------------------------------------------------------------
        MATCH("/voice&/trigger_value", "") { m.reply(&TriggerEvent::value); } break;
        MATCH("/voice&/trigger_number", "") { m.reply(&TriggerEvent::number); } break;
        MATCH("/voice&/trigger_type", "") { m.reply(&TriggerEvent::type); } break;
        MATCH("/voice&/remaining_delay", "") { m.reply(&Voice::getRemainingDelay); } break;
        MATCH("/voice&/source_position", "") { m.reply(&Voice::getSourcePosition); } break;
        #undef MATCH
    }
}

} // namespace sfz
