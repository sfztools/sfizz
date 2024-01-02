// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "CCMap.h"
#include "Config.h"
#include "Defaults.h"
#include "EQDescription.h"
#include "FileId.h"
#include "FilterDescription.h"
#include "FlexEGDescription.h"
#include "LFOCommon.h"
#include "LFODescription.h"
#include "Range.h"
#include "SfzFilter.h"
#include "SfzHelpers.h"
#include "SynthPrivate.h"
#include "FilePool.h"
#include "Curve.h"
#include "MidiState.h"
#include "SynthConfig.h"
#include "TriggerEvent.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "absl/types/span.h"
#include "modulations/ModId.h"
#include "modulations/ModKey.h"
#include "modulations/ModKeyHash.h"
#include "utility/StringViewHelpers.h"
#include <absl/strings/ascii.h>
#include "utility/Size.h"
#include <cstddef>
#include <cstring>
#include <type_traits>
#include <vector>

// TODO: `ccModDepth` and `ccModParameters` are O(N), need better implementation

namespace sfz {
static constexpr unsigned maxIndices = 8;

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

class MessagingHelper
{
public:
    MessagingHelper(Client& client, int delay, const char* path, const char* sig, sfz::Synth::Impl& impl)
    : client(client), impl(impl), delay(delay), path(path), sig(sig)
    {
        indices.reserve(maxIndices);
    }

    enum class ModParam { Depth, Curve, Smooth, Step };

    bool match(const char* pattern, const char* sig)
    {
        indices.clear();
        const char* path_ = this->path;

        while (const char *endp = strchr(pattern, '&')) {
            if (indices.size() == maxIndices)
                return false;

            size_t length = endp - pattern;
            if (strncmp(pattern, path_, length) != 0)
                return false;
            pattern += length;
            path_ += length;

            length = 0;
            while (absl::ascii_isdigit(path_[length]))
                ++length;

            indices.push_back(0);
            if (!absl::SimpleAtoi(absl::string_view(path_, length), &indices.back()))
                return false;

            pattern += 1;
            path_ += length;
        }

        return !strcmp(path_, pattern) && !strcmp(this->sig, sig);
    }

    // These are the reply/reply2 overloads for the (almost) concrete types, that should
    // translate to actual calls to the client.receive(...) method.

    void reply(const std::string& value) { client.receive<'s'>(delay, path, value.data()); }
    void reply(const char* value) { client.receive<'s'>(delay, path, value); }
    void reply(float value) { client.receive<'f'>(delay, path, value); }
    void reply(absl::nullopt_t) { client.receive<'N'>(delay, path, {}); }

    void reply(bool value)
    {
        if (value)
            client.receive<'T'>(delay, path, {});
        else
            client.receive<'F'>(delay, path, {});
    }

    template<class T, typename = std::enable_if_t<std::is_integral<T>::value>>
    void reply(T value)
    {
        if (sizeof(value) <= 4)
            client.receive<'i'>(delay, path, static_cast<int>(value));
        else
            client.receive<'h'>(delay, path, static_cast<long int>(value));
    }
    template<size_t N>
    void reply(const BitArray<N>& array)
    {
        sfizz_blob_t blob { array.data(), static_cast<uint32_t>(array.byte_size()) };
        client.receive<'b'>(delay, path, &blob);
    }

    // Call reply but denormalizes the input if needed by the opcode spec
    template<class T>
    void reply(T value, OpcodeSpec<T> spec) { reply(spec.denormalizeInput(value)); }

    // sfizz specific types
    void reply(sfz::LFOWave wave) { reply(static_cast<int>(wave)); }
    void reply(sfz::SelfMask mode) { reply(mode == SelfMask::mask); }
    void reply(sfz::LoopMode mode)
    {
        switch (mode) {
        case LoopMode::no_loop: reply("no_loop"); break;
        case LoopMode::loop_continuous: reply("loop_continuous"); break;
        case LoopMode::loop_sustain: reply("loop_sustain"); break;
        case LoopMode::one_shot: reply("one_shot"); break;
        }
    }

    void reply(sfz::CrossfadeCurve curve)
    {
        switch (curve) {
        case CrossfadeCurve::gain: reply("gain"); break;
        case CrossfadeCurve::power: reply("power"); break;
        }
    }

    void reply(sfz::Trigger mode)
    {
        switch (mode) {
        case Trigger::attack: reply("attack"); break;
        case Trigger::first: reply("first"); break;
        case Trigger::legato: reply("legato"); break;
        case Trigger::release: reply("release"); break;
        case Trigger::release_key: reply("release_key"); break;
        }
    }

    void reply(sfz::VelocityOverride mode)
    {
        switch (mode) {
        case VelocityOverride::current: reply("current"); break;
        case VelocityOverride::previous: reply("previous"); break;
            }
    }

    void reply(sfz::OffMode mode)
    {
        switch (mode) {
        case OffMode::fast: reply("fast"); break;
        case OffMode::time: reply("time"); break;
        case OffMode::normal: reply("normal"); break;
        }
    }

    void reply(sfz::FilterType type)
    {
        switch (type) {
            case FilterType::kFilterLpf1p: reply("lpf_1p"); break;
            case FilterType::kFilterHpf1p: reply("hpf_1p"); break;
            case FilterType::kFilterLpf2p: reply("lpf_2p"); break;
            case FilterType::kFilterHpf2p: reply("hpf_2p"); break;
            case FilterType::kFilterBpf2p: reply("bpf_2p"); break;
            case FilterType::kFilterBrf2p: reply("brf_2p"); break;
            case FilterType::kFilterBpf1p: reply("bpf_1p"); break;
            case FilterType::kFilterBrf1p: reply("brf_1p"); break;
            case FilterType::kFilterApf1p: reply("apf_1p"); break;
            case FilterType::kFilterLpf2pSv: reply("lpf_2p_sv"); break;
            case FilterType::kFilterHpf2pSv: reply("hpf_2p_sv"); break;
            case FilterType::kFilterBpf2pSv: reply("bpf_2p_sv"); break;
            case FilterType::kFilterBrf2pSv: reply("brf_2p_sv"); break;
            case FilterType::kFilterLpf4p: reply("lpf_4p"); break;
            case FilterType::kFilterHpf4p: reply("hpf_4p"); break;
            case FilterType::kFilterLpf6p: reply("lpf_6p"); break;
            case FilterType::kFilterHpf6p: reply("hpf_6p"); break;
            case FilterType::kFilterPink: reply("pink"); break;
            case FilterType::kFilterLsh: reply("lsh"); break;
            case FilterType::kFilterHsh: reply("hsh"); break;
            case FilterType::kFilterPeq: reply("peq"); break;
            case FilterType::kFilterBpf4p: reply("bpf_4p"); break;
            case FilterType::kFilterBpf6p: reply("bpf_6p"); break;
            case FilterType::kFilterNone: reply("none"); break;
            }
    }

    void reply(sfz::EqType type)
    {
        switch(type) {
        case EqType::kEqNone: reply("none"); break;
        case EqType::kEqPeak: reply("peak"); break;
        case EqType::kEqLshelf: reply("lshelf"); break;
        case EqType::kEqHshelf: reply("hshelf"); break;
        }
    }

    void reply(sfz::TriggerEventType type)
    {
        switch(type) {
        case TriggerEventType::NoteOff: reply("note_off"); break;
        case TriggerEventType::NoteOn: reply("note_on"); break;
        case TriggerEventType::CC: reply("cc"); break;
        }
    }

    // reply2 for pairs (Usually only for float/int ranges)
    template<class T, typename = std::enable_if_t<std::is_integral<T>::value>>
    void reply2(T value1, T value2)
    {
        sfizz_arg_t args[2];
        if (sizeof(value1) <= 4) {
            args[0].i = value1;
            args[1].i = value2;
            client.receive(delay, path, "ii", args);
        } else {
            args[0].h = value1;
            args[1].h = value2;
            client.receive(delay, path, "hh", args);
        }
    }

    void reply2(float value1, float value2)
    {
        sfizz_arg_t args[2];
        args[0].f = value1;
        args[1].f = value2;
        client.receive(delay, path, "ff", args);
    }

    // Now we have some templated reply overloads that decay into "concrete" reply calls
    // but add some logic (e.g. an optional can either reply the value, or send 'N' for
    // null)
    template<class T, class...Args>
    void reply(absl::optional<T> value, Args... args)
    {
        if (!value) {
                client.receive<'N'>(delay, path, {});
            return;
        }

        reply(*value, args...);
    }


    template<class T, class...Args>
    void reply(T* value, Args... args)
    {
        if (!value) {
            client.receive<'N'>(delay, path, {});
            return;
        }

        reply(*value, args...);
    }

    template<class T>
    void reply(std::shared_ptr<T> value) { reply(value.get()); }

    template<class T>
    void reply(sfz::UncheckedRange<T> range) { reply2(range.getStart(), range.getEnd()); }

    template<class T>
    void reply(absl::optional<T> value, T def) { reply(value.value_or(def)); }

    template<class T, class...Args>
    void reply(const sfz::ModifierCurvePair<T>& modCurve, ModParam which, Args...args)
    {
        if (auto region = getRegion()) {
            switch (which) {
            case ModParam::Curve: reply(modCurve.curve); break;
            default: reply(modCurve.modifier, args...); break;
            }
        }
    }

    template<class...Args>
    void reply(const sfz::ModKey::Parameters& params, ModParam which, Args...args)
    {
        if (auto region = getRegion()) {
            switch (which) {
            case ModParam::Depth: break;
            case ModParam::Curve: reply(params.curve); break;
            case ModParam::Smooth: reply(params.smooth); break;
            case ModParam::Step: reply(params.step, args...); break;
            }
        }
    }

    template<class T, class...Args>
    void reply(CCMap<T> map, Args...args) { reply(map, true, args...); }

    template<class T, class...Args>
    void reply(CCMap<T> map, bool useDefault, Args...args)
    {
        if (useDefault)
            reply(map.getWithDefault(indices.back()), args...);
        else if (map.contains(indices.back()))
            reply(map.get(indices.back()), args...);
        else
            client.receive<'N'>(delay, path, {});
    }

    // Below are all methods that will fetch various data structure elements (regions,
    // egs, ...), check that they exist, and call an actual "concrete" reply
    // implementation. They use pointer-to-member variables, which allow the compiler to
    // dispatch to the correct logic. For example, if the pointer-to-member is
    // `sfz::Region::*`, we aim at sending the value of a member of the sfz::Region
    // struct. The first thing is to read the index of the region and fetch it from the
    // sfizz data structure using `getRegion(...)`, and then apply the pointer-to-member
    // to this particular region to send the value.

    // Adding new dispatching overloads seemed simple enough to this point, although I
    // haven't found a nice way to the same with pointer-to-member function.
    // TODO: maybe a semi-templated overload that check that if the member is a function,
    // TODO: and call it with `args...` could be an approach to try.

    template<class T, class... Args>
    void reply(T sfz::Region::* member, Args... args)
    {
        if (auto region = getRegion())
            reply((*region).*member, args...);
    }

    template<class T, class...Args>
    void reply(const sfz::EGDescription& eg, T sfz::EGDescription::* member, Args...args)
    {
        reply(eg.*member, args...);
    }

    template<class T, class... Args>
    void reply(T sfz::FilterDescription::* member, Args... args)
    {
        if (auto region = getRegion())
            if (auto filter = getFilter(*region))
                reply((*filter).*member, args...);
    }

    template<class T, class... Args>
    void reply(T sfz::EQDescription::* member, Args... args)
    {
        if (auto region = getRegion())
            if (auto eq = getEQ(*region))
                reply((*eq).*member, args...);
    }

    template<class T, class... Args>
    void reply(T sfz::LFODescription::* member, Args... args)
    {
        if (auto region = getRegion())
            if (auto lfo = getLFO(*region))
                reply((*lfo).*member, args...);
    }

    template<class T, class... Args>
    void reply(T sfz::LFODescription::Sub::* member, Args... args)
    {
        if (auto region = getRegion())
            if (auto lfo = getLFO(*region))
                if (!lfo->sub.empty())
                    reply((lfo->sub[0]).*member, args...);
    }

    template<class T, class... Args>
    void reply(T sfz::FlexEGPoint::* member, Args... args)
    {
        if (auto region = getRegion())
            if (auto eg = getEG(*region))
                if (auto point = getEGPoint(*eg))
                    reply((*point).*member, args...);
    }

    template<class T, class... Args>
    void reply(T sfz::TriggerEvent::* member, Args... args)
    {
        if (auto voice = getVoice())
            reply((*voice).getTriggerEvent().*member, args...);
    }

    template<class T, class... Args>
    void reply(T sfz::Voice::* member, Args... args)
    {
        if (auto voice = getVoice()) {
            reply((*voice.*member)(args...)); // Voice only has callables
        }
    }

    template<class...Args>
    void reply(ModId id, ModParam param, Args...args)
    {
        if (auto region = getRegion()) {
            int cc = static_cast<int> (indices.back());
            switch (id){
            case ModId::FilCutoff:
            case ModId::FilGain:
                switch (param) {
                case ModParam::Depth: reply(region->ccModDepth(cc, id, indices[1]), args...); break;
                default: reply(region->ccModParameters(cc, id, indices[1]), param, args...); break;
                } break;
            default:
                switch (param) {
                case ModParam::Depth: reply(region->ccModDepth(cc, id), args...); break;
                default: reply(region->ccModParameters(cc, id), param, args...); break;
                }
            }
        }
    }

    // Validate and fetch elements from the sfizz data structures. By default, we kind of
    // assume that regions/voices will be the first index, CCs will be the last, and
    // EQ/Filter/.. will be in-between.
    sfz::Region* getRegion(absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[0]);
        if (idx >= impl.layers_.size())
            return {};

        Layer& layer = *impl.layers_[idx];
        return &layer.getRegion();
    }

    sfz::FilterDescription* getFilter(sfz::Region& region, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[1]);
        if (region.filters.size() <= idx)
            return {};

        return &region.filters[idx];
    }

    sfz::EQDescription* getEQ(sfz::Region& region, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[1]);
        if (region.equalizers.size() <= idx)
            return {};

        return &region.equalizers[idx];
    }

    sfz::LFODescription* getLFO(sfz::Region& region, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[1]);
        if (region.lfos.size() <= idx)
            return {};

        return &region.lfos[idx];
    }

    sfz::LFODescription::Sub* getLFOSub(sfz::LFODescription& lfo, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[2]);
        if (lfo.sub.size() <= idx)
            return {};

        return &lfo.sub[idx];
    }

    sfz::FlexEGDescription* getEG(sfz::Region& region, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[1]);
        if (region.flexEGs.size() <= idx)
            return {};

        return &region.flexEGs[idx];
    }

    sfz::FlexEGPoint* getEGPoint(sfz::FlexEGDescription& desc, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[2]) + 1;
        if (desc.points.size() <= idx)
            return {};

        return &desc.points[idx];
    }

    sfz::Voice* getVoice(absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[0]);
        if (static_cast<int>(idx) >= impl.numVoices_)
            return {};

        auto& voice = impl.voiceManager_[idx];
        if (voice.isFree())
            return {};

        return &voice;
    }

    // Helpers to get and check the values of the indices
    template<class T = unsigned>
    absl::optional<T> index(int i)
    {
        if (i <= ssize(indices))
            return static_cast<T>(indices[i]);

        return absl::nullopt;
    }

    absl::optional<int> sindex(int i) { return index<int>(i); }

    absl::optional<int> checkCC(int i = 0)
    {
        auto cc = index(i);
        return cc <= config::numCCs ? cc : absl::nullopt;
    }

    absl::optional<int> checkNote(int i = 0)
    {
        auto note = sindex(i);
        return note <= 127 ? note : absl::nullopt;
    }

private:
    Client& client;
    std::vector<unsigned> indices;
    sfz::Synth::Impl& impl;
    int delay;
    const char* path;
    const char* sig;
};

void sfz::Synth::dispatchMessage(Client& client, int delay, const char* path, const char* sig, const sfizz_arg_t* args)
{
    Impl& impl = *impl_;
    MessagingHelper m {client, delay, path, sig, impl};
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
        MATCH("/region&/delay_random", "") { m.reply(&Region::delayRandom); } break;
        MATCH("/region&/sample", "") { if (auto region = m.getRegion()) { m.reply(region->sampleId->filename()); } } break;
        MATCH("/region&/direction", "") { if (auto region = m.getRegion()) { m.reply(region->sampleId->isReverse() ? "reverse" : "forward"); } } break;
        MATCH("/region&/delay_cc&", "") { m.reply(&Region::delayCC); } break;
        MATCH("/region&/offset", "") { m.reply(&Region::offset); } break;
        MATCH("/region&/offset_random", "") { m.reply(&Region::offsetRandom); } break;
        MATCH("/region&/offset_cc&", "") { m.reply(&Region::offsetCC); } break;
        MATCH("/region&/end", "") { m.reply(&Region::sampleEnd); } break;
        MATCH("/region&/end_cc&", "") { m.reply(&Region::endCC); } break;
        MATCH("/region&/enabled", "") { if (auto region = m.getRegion()) { m.reply(!region->disabled()); } } break;
        MATCH("/region&/trigger_on_note", "") { m.reply(&Region::triggerOnNote); } break;
        MATCH("/region&/trigger_on_cc", "") { m.reply(&Region::triggerOnCC); } break;
        MATCH("/region&/use_timer_range", "") { m.reply(&Region::useTimerRange); } break;
        MATCH("/region&/count", "") { m.reply(&Region::sampleCount); } break;
        MATCH("/region&/loop_range", "") { m.reply(&Region::loopRange); } break;
        MATCH("/region&/loop_start_cc&", "") { m.reply(&Region::loopStartCC); } break;
        MATCH("/region&/loop_end_cc&", "") { m.reply(&Region::loopEndCC); } break;
        MATCH("/region&/loop_mode", "") { m.reply(&Region::loopMode, LoopMode::no_loop); } break;
        MATCH("/region&/loop_crossfade", "") { m.reply(&Region::loopCrossfade); } break;
        MATCH("/region&/loop_count", "") { m.reply(&Region::loopCount); } break;
        MATCH("/region&/output", "") { m.reply(&Region::output); } break;
        MATCH("/region&/group", "") { m.reply(&Region::group); } break;
        MATCH("/region&/off_by", "") { m.reply(&Region::offBy); } break;
        MATCH("/region&/off_mode", "") { m.reply(&Region::offMode); } break;
        MATCH("/region&/key_range", "") { m.reply(&Region::keyRange); } break;
        MATCH("/region&/off_time", "") { m.reply(&Region::offTime); } break;
        MATCH("/region&/pitch_keycenter", "") { m.reply(&Region::pitchKeycenter); } break;
        MATCH("/region&/vel_range", "") { m.reply(&Region::velocityRange); } break;
        MATCH("/region&/bend_range", "") { m.reply(&Region::bendRange); } break;
        MATCH("/region&/program_range", "") { m.reply(&Region::programRange); } break;
        MATCH("/region&/cc_range&", "") { m.reply(&Region::ccConditions); } break;
        MATCH("/region&/sw_last", "") {
            if (auto region = m.getRegion()) {
                if (region->lastKeyswitch) m.reply(region->lastKeyswitch);
                else m.reply(region->lastKeyswitchRange);
            }
        } break;
        MATCH("/region&/sw_label", "") { m.reply(&Region::keyswitchLabel); } break;
        MATCH("/region&/sw_up", "") { m.reply(&Region::upKeyswitch); } break;
        MATCH("/region&/sw_down", "") { m.reply(&Region::downKeyswitch); } break;
        MATCH("/region&/sw_previous", "") { m.reply(&Region::previousKeyswitch); } break;
        MATCH("/region&/sw_vel", "") { m.reply(&Region::velocityOverride); } break;
        MATCH("/region&/chanaft_range", "") { m.reply(&Region::aftertouchRange); } break;
        MATCH("/region&/polyaft_range", "") { m.reply(&Region::polyAftertouchRange); } break;
        MATCH("/region&/bpm_range", "") { m.reply(&Region::bpmRange); } break;
        MATCH("/region&/rand_range", "") { m.reply(&Region::randRange); } break;
        MATCH("/region&/seq_length", "") { m.reply(&Region::sequenceLength); } break;
        MATCH("/region&/seq_position", "") { m.reply(&Region::sequencePosition); } break;
        MATCH("/region&/trigger", "") { m.reply(&Region::trigger); } break;
        MATCH("/region&/start_cc_range&", "") { m.reply(&Region::ccTriggers, false); } break;
        MATCH("/region&/volume", "") { m.reply(&Region::volume); } break;
        MATCH("/region&/volume_cc&", "") { m.reply(ModId::Volume, ModParam::Depth); } break;
        MATCH("/region&/volume_stepcc&", "") { m.reply(ModId::Volume, ModParam::Step); } break;
        MATCH("/region&/volume_smoothcc&", "") { m.reply(ModId::Volume, ModParam::Smooth); } break;
        MATCH("/region&/volume_curvecc&", "") { m.reply(ModId::Volume, ModParam::Curve); } break;
        MATCH("/region&/pan", "") { m.reply(&Region::pan, Default::pan); } break;
        MATCH("/region&/pan_cc&", "") { m.reply(ModId::Pan, ModParam::Depth, Default::pan); } break;
        MATCH("/region&/pan_stepcc&", "") { m.reply(ModId::Pan, ModParam::Step, Default::pan); } break;
        MATCH("/region&/pan_smoothcc&", "") { m.reply(ModId::Pan, ModParam::Smooth, Default::pan); } break;
        MATCH("/region&/pan_curvecc&", "") { m.reply(ModId::Pan, ModParam::Curve, Default::pan); } break;
        MATCH("/region&/width", "") { m.reply(&Region::width, Default::width); } break;
        MATCH("/region&/width_cc&", "") { m.reply(ModId::Width, ModParam::Depth, Default::width); } break;
        MATCH("/region&/width_stepcc&", "") { m.reply(ModId::Width, ModParam::Step, Default::width); } break;
        MATCH("/region&/width_smoothcc&", "") { m.reply(ModId::Width, ModParam::Smooth, Default::width); } break;
        MATCH("/region&/width_curvecc&", "") { m.reply(ModId::Width, ModParam::Curve, Default::width); } break;
        MATCH("/region&/timer_range", "") { m.reply(&Region::timerRange); } break;
        MATCH("/region&/position", "") { m.reply(&Region::position, Default::position); } break;
        MATCH("/region&/position_cc&", "") { m.reply(ModId::Position, ModParam::Depth, Default::position); } break;
        MATCH("/region&/position_stepcc&", "") { m.reply(ModId::Position, ModParam::Step, Default::position); } break;
        MATCH("/region&/position_smoothcc&", "") { m.reply(ModId::Position, ModParam::Smooth, Default::position); } break;
        MATCH("/region&/position_curvecc&", "") { m.reply(ModId::Position, ModParam::Curve, Default::position); } break;
        MATCH("/region&/amplitude", "") { m.reply(&Region::amplitude, Default::amplitude); } break;
        MATCH("/region&/amplitude_cc&", "") { m.reply(ModId::Amplitude, ModParam::Depth, Default::amplitude); } break;
        MATCH("/region&/amplitude_stepcc&", "") { m.reply(ModId::Amplitude, ModParam::Step, Default::amplitude); } break;
        MATCH("/region&/amplitude_smoothcc&", "") { m.reply(ModId::Amplitude, ModParam::Smooth, Default::amplitude); } break;
        MATCH("/region&/amplitude_curvecc&", "") { m.reply(ModId::Amplitude, ModParam::Curve, Default::amplitude); } break;
        MATCH("/region&/amp_keycenter", "") { m.reply(&Region::ampKeycenter); } break;
        MATCH("/region&/amp_keytrack", "") { m.reply(&Region::ampKeytrack); } break;
        MATCH("/region&/amp_veltrack", "") { m.reply(&Region::ampVeltrack, Default::ampVeltrack); } break;
        MATCH("/region&/amp_veltrack_cc&", "") { m.reply(&Region::ampVeltrackCC, false, ModParam::Depth, Default::ampVeltrackMod); } break;
        MATCH("/region&/amp_veltrack_curvecc&", "") { m.reply(&Region::ampVeltrackCC, false, ModParam::Curve, Default::ampVeltrackMod); } break;
        MATCH("/region&/amp_random", "") { m.reply(&Region::ampRandom); } break;
        MATCH("/region&/xfin_key_range", "") { m.reply(&Region::crossfadeKeyInRange); } break;
        MATCH("/region&/xfout_key_range", "") { m.reply(&Region::crossfadeKeyOutRange); } break;
        MATCH("/region&/xfin_vel_range", "") { m.reply(&Region::crossfadeVelInRange); } break;
        MATCH("/region&/xfout_vel_range", "") { m.reply(&Region::crossfadeVelOutRange); } break;
        MATCH("/region&/xfin_cc_range&", "") { m.reply(&Region::crossfadeCCInRange, false); } break;
        MATCH("/region&/xfout_cc_range&", "") { m.reply(&Region::crossfadeCCOutRange, false); } break;
        MATCH("/region&/xf_keycurve", "") { m.reply(&Region::crossfadeKeyCurve); } break;
        MATCH("/region&/xf_velcurve", "") { m.reply(&Region::crossfadeVelCurve); } break;
        MATCH("/region&/xf_cccurve", "") { m.reply(&Region::crossfadeCCCurve); } break;
        MATCH("/region&/global_volume", "") { m.reply(&Region::globalVolume); } break;
        MATCH("/region&/master_volume", "") { m.reply(&Region::masterVolume); } break;
        MATCH("/region&/group_volume", "") { m.reply(&Region::groupVolume); } break;
        MATCH("/region&/global_amplitude", "") { m.reply(&Region::globalAmplitude, Default::amplitude); } break;
        MATCH("/region&/master_amplitude", "") { m.reply(&Region::masterAmplitude, Default::amplitude); } break;
        MATCH("/region&/group_amplitude", "") { m.reply(&Region::groupAmplitude, Default::amplitude); } break;
        MATCH("/region&/pitch_keytrack", "") { m.reply(&Region::pitchKeytrack); } break;
        MATCH("/region&/pitch_veltrack", "") { m.reply(&Region::pitchVeltrack); } break;
        MATCH("/region&/pitch_veltrack_cc&", "") { m.reply(&Region::pitchVeltrackCC, false, ModParam::Depth); } break;
        MATCH("/region&/pitch_veltrack_curvecc&", "") { m.reply(&Region::pitchVeltrackCC, false, ModParam::Curve); } break;
        MATCH("/region&/pitch_random", "") { m.reply(&Region::pitchRandom); } break;
        MATCH("/region&/transpose", "") { m.reply(&Region::transpose); } break;
        MATCH("/region&/pitch", "") { m.reply(&Region::pitch); } break;
        MATCH("/region&/pitch_cc&", "") { m.reply(ModId::Pitch, ModParam::Depth, Default::pitch); } break;
        MATCH("/region&/pitch_stepcc&", "") { m.reply(ModId::Pitch, ModParam::Step, Default::pitch); } break;
        MATCH("/region&/pitch_smoothcc&", "") { m.reply(ModId::Pitch, ModParam::Smooth, Default::pitch); } break;
        MATCH("/region&/pitch_curvecc&", "") { m.reply(ModId::Pitch, ModParam::Curve, Default::pitch); } break;
        MATCH("/region&/bend_up", "") { m.reply(&Region::bendUp); } break;
        MATCH("/region&/bend_down", "") { m.reply(&Region::bendDown); } break;
        MATCH("/region&/bend_step", "") { m.reply(&Region::bendStep); } break;
        MATCH("/region&/bend_smooth", "") { m.reply(&Region::bendSmooth); } break;
        MATCH("/region&/ampeg_attack", "") { m.reply(&Region::amplitudeEG, &EGDescription::attack); } break;
        MATCH("/region&/ampeg_delay", "") { m.reply(&Region::amplitudeEG, &EGDescription::delay); } break;
        MATCH("/region&/ampeg_decay", "") { m.reply(&Region::amplitudeEG, &EGDescription::decay); } break;
        MATCH("/region&/ampeg_hold", "") { m.reply(&Region::amplitudeEG, &EGDescription::hold); } break;
        MATCH("/region&/ampeg_release", "") { m.reply(&Region::amplitudeEG, &EGDescription::release); } break;
        MATCH("/region&/ampeg_start", "") { m.reply(&Region::amplitudeEG, &EGDescription::start, Default::egPercentMod); } break;
        MATCH("/region&/ampeg_sustain", "") { m.reply(&Region::amplitudeEG, &EGDescription::sustain, Default::egPercentMod); } break;
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
        MATCH("/region&/note_polyphony", "") { m.reply(&Region::notePolyphony); } break;
        MATCH("/region&/rt_dead", "") { m.reply(&Region::rtDead); } break;
        MATCH("/region&/sustain_sw", "") { m.reply(&Region::checkSustain); } break;
        MATCH("/region&/sostenuto_sw", "") { m.reply(&Region::checkSostenuto); } break;
        MATCH("/region&/sustain_cc", "") { m.reply(&Region::sustainCC); } break;
        MATCH("/region&/sostenuto_cc", "") { m.reply(&Region::sostenutoCC); } break;
        MATCH("/region&/sustain_lo", "") { m.reply(&Region::sustainThreshold); } break;
        MATCH("/region&/sostenuto_lo", "") { m.reply(&Region::sostenutoThreshold); } break;
        MATCH("/region&/note_selfmask", "") { m.reply(&Region::selfMask); } break;
        MATCH("/region&/oscillator_phase", "") { m.reply(&Region::oscillatorPhase); } break;
        MATCH("/region&/oscillator_quality", "") { m.reply(&Region::oscillatorQuality); } break;
        MATCH("/region&/oscillator_mode", "") { m.reply(&Region::oscillatorMode); } break;
        MATCH("/region&/oscillator_multi", "") { m.reply(&Region::oscillatorMulti); } break;
        MATCH("/region&/oscillator_detune", "") { m.reply(&Region::oscillatorDetune); } break;
        MATCH("/region&/oscillator_mod_depth", "") { m.reply(&Region::oscillatorModDepth, Default::oscillatorModDepth); } break;
        // TODO: detune cc, mod depth cc

        MATCH("/region&/effect&", "") {
            if (auto region = m.getRegion())
                if (auto effectIdx = m.sindex(1))
                    if (effectIdx > 0 && effectIdx < ssize(region->gainToEffect))
                        m.reply(region->gainToEffect[*effectIdx], Default::effect);
        } break;
        MATCH("/region&/filter&/cutoff", "") { m.reply(&FilterDescription::cutoff); } break;
        MATCH("/region&/filter&/cutoff_cc&", "") { m.reply(ModId::FilCutoff, ModParam::Depth); } break;
        MATCH("/region&/filter&/cutoff_curvecc&", "") { m.reply(ModId::FilCutoff, ModParam::Curve); } break;
        MATCH("/region&/filter&/cutoff_stepcc&", "") { m.reply(ModId::FilCutoff, ModParam::Step); } break;
        MATCH("/region&/filter&/cutoff_smoothcc&", "") { m.reply(ModId::FilCutoff, ModParam::Smooth); } break;
        MATCH("/region&/filter&/resonance", "") { m.reply(&FilterDescription::resonance); } break;
        MATCH("/region&/filter&/gain", "") { m.reply(&FilterDescription::gain); } break;
        MATCH("/region&/filter&/keycenter", "") { m.reply(&FilterDescription::keycenter); } break;
        MATCH("/region&/filter&/keytrack", "") { m.reply(&FilterDescription::keytrack); } break;
        MATCH("/region&/filter&/veltrack", "") { m.reply(&FilterDescription::veltrack); } break;
        MATCH("/region&/filter&/veltrack_cc&", "") { m.reply(&FilterDescription::veltrackCC, ModParam::Depth); } break;
        MATCH("/region&/filter&/veltrack_curvecc&", "") { m.reply(&FilterDescription::veltrackCC, ModParam::Curve); } break;
        MATCH("/region&/filter&/type", "") { m.reply(&FilterDescription::type); } break;
        //----------------------------------------------------------------------
        MATCH("/region&/eq&/gain", "") { m.reply(&EQDescription::gain); } break;
        MATCH("/region&/eq&/bandwidth", "") { m.reply(&EQDescription::bandwidth); } break;
        MATCH("/region&/eq&/frequency", "") { m.reply(&EQDescription::frequency); } break;
        MATCH("/region&/eq&/vel&freq", "") { m.reply(&EQDescription::vel2frequency); } break;
        MATCH("/region&/eq&/vel&gain", "") { m.reply(&EQDescription::vel2gain); } break;
        MATCH("/region&/eq&/type", "") { m.reply(&EQDescription::type); } break;
        //----------------------------------------------------------------------
        MATCH("/region&/lfo&/wave", "") { m.reply(&LFODescription::Sub::wave); } break;
        //----------------------------------------------------------------------
        MATCH("/region&/eg&/point&/time", "") { m.reply(&FlexEGPoint::time); } break;
        MATCH("/region&/eg&/point&/time_cc&", "") { m.reply(&FlexEGPoint::ccTime); } break;
        MATCH("/region&/eg&/point&/level", "") { m.reply(&FlexEGPoint::level); } break;
        MATCH("/region&/eg&/point&/level_cc&", "") { m.reply(&FlexEGPoint::ccLevel); } break;
        //----------------------------------------------------------------------
        MATCH("/voice&/trigger_value", "") { m.reply(&TriggerEvent::value); } break;
        MATCH("/voice&/trigger_number", "") { m.reply(&TriggerEvent::number); } break;
        MATCH("/voice&/trigger_type", "") { m.reply(&TriggerEvent::type); } break;
        MATCH("/voice&/remaining_delay", "") { m.reply(&Voice::getRemainingDelay); } break;
        MATCH("/voice&/source_position", "") { m.reply(&Voice::getSourcePosition); } break;
        //----------------------------------------------------------------------

        // Setting values
        // Note: all these must be rt-safe within the parseOpcode method in region

        MATCH("/sample_quality", "i") {
            impl.resources_.getSynthConfig().liveSampleQuality =
                Opcode::transform(Default::sampleQuality, static_cast<int>(args[0].i));
        } break;

        MATCH("/oscillator_quality", "i") {
            impl.resources_.getSynthConfig().liveOscillatorQuality =
                Opcode::transform(Default::oscillatorQuality, static_cast<int>(args[0].i));
        } break;

        MATCH("/freewheeling_sample_quality", "i") {
            impl.resources_.getSynthConfig().freeWheelingSampleQuality =
                Opcode::transform(Default::freewheelingSampleQuality, static_cast<int>(args[0].i));
        } break;

        MATCH("/freewheeling_oscillator_quality", "i") {
            impl.resources_.getSynthConfig().freeWheelingOscillatorQuality =
                Opcode::transform(Default::freewheelingOscillatorQuality, static_cast<int>(args[0].i));
        } break;

        MATCH("/sustain_cancels_release", "T") {
            impl.resources_.getSynthConfig().sustainCancelsRelease = true;
        } break;

        MATCH("/sustain_cancels_release", "F") {
            impl.resources_.getSynthConfig().sustainCancelsRelease = false;
        } break;

        MATCH("/region&/pitch_keycenter", "i") {
            if (auto region = m.getRegion())
                region->pitchKeycenter = Opcode::transform(Default::key, args[0].i);
        } break;

        MATCH("/region&/loop_mode", "s") {
            if (auto region = m.getRegion())
                region->loopMode = Opcode::readOptional(Default::loopMode, args[0].s);
        } break;

        MATCH("/region&/filter&/type", "s") {
            if (auto region = m.getRegion())
                if (auto filter = m.getFilter(*region))
                    filter->type = Opcode::read(Default::filter, args[0].s);
        } break;

        MATCH("/region&/lfo&/wave", "i") {
            if (auto region = m.getRegion())
                if (auto lfo = m.getLFO(*region))
                    if (!lfo->sub.empty())
                        lfo->sub[0].wave = Opcode::transform(Default::lfoWave, args[0].i);
        } break;

        MATCH("/region&/lfo&/wave&", "i") {
            if (auto region = m.getRegion())
                if (auto lfo = m.getLFO(*region))
                    if (auto sub = m.getLFOSub(*lfo))
                        sub->wave = Opcode::transform(Default::lfoWave, args[0].i);
        } break;

        #undef MATCH
    }
}

} // namespace sfz
