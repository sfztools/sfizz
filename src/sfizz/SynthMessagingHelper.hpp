#pragma once

#include "CCMap.h"
#include "Config.h"
#include "Defaults.h"
#include "EQDescription.h"
#include "FileId.h"
#include "FilterDescription.h"
#include "FlexEGDescription.h"
#include "LFOCommon.h"
#include "LFODescription.h"
#include "Opcode.h"
#include "Range.h"
#include "SfzFilter.h"
#include "SfzHelpers.h"
#include "SynthPrivate.h"
#include "FilePool.h"
#include "Curve.h"
#include "MidiState.h"
#include "SynthConfig.h"
#include "TriggerEvent.h"
#include "SynthPrivate.h"
#include "utility/Size.h"
#include <type_traits>
#include <invoke.hpp/invoke.hpp>

namespace inv = invoke_hpp;

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

template <class T>
using IntegralNotBool = std::enable_if_t<std::is_integral<T>::value && !std::is_same<T, bool>::value>;

template <class T>
using BoolOrNotIntegral = std::enable_if_t<!std::is_integral<T>::value || std::is_same<T, bool>::value>;

namespace sfz {

static constexpr unsigned maxIndices = 8;

class MessagingHelper {
public:
    MessagingHelper(Client& client, int delay, const char* path, const char* sig, const sfizz_arg_t* args, Synth::Impl& impl)
        : client(client)
        , impl(impl)
        , delay(delay)
        , path(path)
        , sig(sig)
        , request_args(args)
    {
        indices.reserve(maxIndices);
    }

    enum class ModParam { Depth, Curve, Smooth, Step };

    bool match(const char* pattern, const char* sig)
    {
        indices.clear();
        const char* path_ = this->path;

        while (const char* endp = strchr(pattern, '&')) {
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
    // void reply(float value) { client.receive<'f'>(delay, path, value); }
    void reply(const float& value) { client.receive<'f'>(delay, path, value); }
    void reply(absl::nullopt_t) { client.receive<'N'>(delay, path, {}); }

    void reply(const bool& value)
    {
        if (value)
            client.receive<'T'>(delay, path, {});
        else
            client.receive<'F'>(delay, path, {});
    }

    template <class T, typename = std::enable_if_t<std::is_integral<T>::value>>
    void reply(const T& value)
    {
        if (sizeof(value) <= 4)
            client.receive<'i'>(delay, path, static_cast<int>(value));
        else
            client.receive<'h'>(delay, path, static_cast<long int>(value));
    }
    template <size_t N>
    void reply(const BitArray<N>& array)
    {
        sfizz_blob_t blob { array.data(), static_cast<uint32_t>(array.byte_size()) };
        client.receive<'b'>(delay, path, &blob);
    }

    // Call reply but denormalizes the input if needed by the opcode spec
    template <class T>
    void reply(const T& value, const OpcodeSpec<T>& spec) { reply(spec.denormalizeInput(value)); }

    template <class T>
    BoolOrNotIntegral<T> set(T& target, const OpcodeSpec<T>& spec) { target = Opcode::read(spec, request_args[0].s); }

    // sfizz specific types
    void reply(const LFOWave& wave) { reply(static_cast<int>(wave)); }
    void reply(const SelfMask& mode) { reply(mode == SelfMask::mask); }
    void reply(const LoopMode& mode)
    {
        switch (mode) {
        case LoopMode::no_loop: reply("no_loop"); break;
        case LoopMode::loop_continuous: reply("loop_continuous"); break;
        case LoopMode::loop_sustain: reply("loop_sustain"); break;
        case LoopMode::one_shot: reply("one_shot"); break;
        }
    }

    void reply(const CrossfadeCurve& curve)
    {
        switch (curve) {
        case CrossfadeCurve::gain: reply("gain"); break;
        case CrossfadeCurve::power: reply("power"); break;
        }
    }

    void reply(const Trigger& mode)
    {
        switch (mode) {
        case Trigger::attack: reply("attack"); break;
        case Trigger::first: reply("first"); break;
        case Trigger::legato: reply("legato"); break;
        case Trigger::release: reply("release"); break;
        case Trigger::release_key: reply("release_key"); break;
        }
    }

    void reply(const VelocityOverride& mode)
    {
        switch (mode) {
        case VelocityOverride::current: reply("current"); break;
        case VelocityOverride::previous: reply("previous"); break;
        }
    }

    void reply(const OffMode& mode)
    {
        switch (mode) {
        case OffMode::fast: reply("fast"); break;
        case OffMode::time: reply("time"); break;
        case OffMode::normal: reply("normal"); break;
        }
    }

    void reply(const FilterType& type)
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

    void reply(const EqType& type)
    {
        switch (type) {
        case EqType::kEqNone: reply("none"); break;
        case EqType::kEqPeak: reply("peak"); break;
        case EqType::kEqLshelf: reply("lshelf"); break;
        case EqType::kEqHshelf: reply("hshelf"); break;
        }
    }

    void reply(const TriggerEventType& type)
    {
        switch (type) {
        case TriggerEventType::NoteOff: reply("note_off"); break;
        case TriggerEventType::NoteOn: reply("note_on"); break;
        case TriggerEventType::CC: reply("cc"); break;
        }
    }

    // reply2 for pairs (Usually only for float/int ranges)
    template <class T, typename = std::enable_if_t<std::is_integral<T>::value>>
    void reply2(T value1, T value2)
    {
        if (sizeof(value1) <= 4) {
            client.receive<'i', 'i'>(delay, path, value1, value2);
        } else {
            client.receive<'h', 'h'>(delay, path, value1, value2);
        }
    }

    void reply2(float value1, float value2)
    {
        client.receive<'f', 'f'>(delay, path, value1, value2);
    }

    void set(std::string& target) { target = request_args[0].s; }
    void set(float& target, const OpcodeSpec<float>& spec) { target = Opcode::transform(spec, request_args[0].f); }
    void set(float& target) { target = request_args[0].f; }
    void set(LFOWave& target, const OpcodeSpec<LFOWave>& spec) { target = Opcode::transform(spec, request_args[0].i); }
    void set(bool& target, const OpcodeSpec<bool>& spec)
    {
        if (sig[0] == 'T') {
            target = true;
        } else if (sig[0] == 'F') {
            target = false;
        } else {
            target = Opcode::read(spec, request_args[0].s);
        }
    }

    template <class T>
    IntegralNotBool<T> set(T& target, const OpcodeSpec<T>& spec)
    {
        if (sizeof(target) <= 4)
            target = Opcode::transform(spec, request_args[0].i);
        else
            target = Opcode::transform(spec, request_args[0].h);
    }

    // Now we have some templated reply overloads that decay into "concrete" reply calls
    // but add some logic (e.g. an optional can either reply the value, or send 'N' for
    // null)
    template <class T, class... Args>
    void reply(const absl::optional<T>& value, Args... args)
    {
        if (!value) {
            client.receive<'N'>(delay, path, {});
            return;
        }

        reply(*value, std::forward<Args>(args)...);
    }

    template <class T, class... Args>
    void reply(T* value, Args... args)
    {
        if (!value) {
            client.receive<'N'>(delay, path, {});
            return;
        }

        reply(*value, std::forward<Args>(args)...);
    }

    template <class T>
    void reply(const std::shared_ptr<T>& value) { reply(value.get()); }

    template <class T>
    void reply(const UncheckedRange<T>& range) { reply2(range.getStart(), range.getEnd()); }
    template <class T>
    void reply(const UncheckedRange<T>& range, const OpcodeSpec<T>& startSpec, const OpcodeSpec<T>& endSpec)
    {
        reply2(startSpec.denormalizeInput(range.getStart()), endSpec.denormalizeInput(range.getEnd()));
    }

    template <class T>
    IntegralNotBool<T> set(UncheckedRange<T>& range, const OpcodeSpec<T>& startSpec, const OpcodeSpec<T>& endSpec)
    {
        if (sizeof(range.getStart()) <= 4) {
            range.setStart(Opcode::transform(startSpec, request_args[0].i));
            range.setEnd(Opcode::transform(endSpec, request_args[1].i));
        } else {
            range.setStart(Opcode::transform(startSpec, request_args[0].h));
            range.setEnd(Opcode::transform(endSpec, request_args[1].h));
        }
    }

    template <class T>
    BoolOrNotIntegral<T> set(UncheckedRange<T>& range, const OpcodeSpec<T>& startSpec, const OpcodeSpec<T>& endSpec)
    {
        if (sizeof(range.getStart()) <= 4) {
            range.setStart(Opcode::transform(startSpec, request_args[0].f));
            range.setEnd(Opcode::transform(endSpec, request_args[1].f));
        } else {
            range.setStart(Opcode::transform(startSpec, request_args[0].d));
            range.setEnd(Opcode::transform(endSpec, request_args[1].d));
        }
    }

    template <class T>
    IntegralNotBool<T> set(UncheckedRange<T>& range)
    {
        if (sizeof(range.getStart()) <= 4) {
            range.setStart(request_args[0].i);
            range.setEnd(request_args[1].i);
        } else {
            range.setStart(request_args[0].h);
            range.setEnd(request_args[1].h);
        }
    }

    template <class T>
    BoolOrNotIntegral<T> set(UncheckedRange<T>& range)
    {
        if (sizeof(range.getStart()) <= 4) {
            range.setStart(request_args[0].f);
            range.setEnd(request_args[1].f);
        } else {
            range.setStart(request_args[0].d);
            range.setEnd(request_args[1].d);
        }
    }

    template <class T>
    void reply(const absl::optional<T>& value, T&& def) { reply(value.value_or(def)); }

    template <class T, class...Args>
    void set(absl::optional<T>& member, Args&&...args)
    {
        if (sig[0] == 'N') {
            member.reset();
        } else if (member.has_value()) {
            set(*member, std::forward<Args>(args)...);
        } else {
            set(member.emplace(), std::forward<Args>(args)...);
        }
    }

    template <class T, class... Args>
    void reply(const ModifierCurvePair<T>& modCurve, ModParam which, Args&&... args)
    {
        switch (which) {
        case ModParam::Curve: reply(modCurve.curve); break;
        default: reply(modCurve.modifier, std::forward<Args>(args)...); break;
        }
    }

    template <class T, class... Args>
    void set(ModifierCurvePair<T>& modCurve, ModParam which, Args&&... args)
    {
        switch (which) {
        case ModParam::Curve: modCurve.curve = request_args[0].i; break;
        default: set(modCurve.modifier, args...); break;
        }
    }

    template <class... Args>
    void reply(const ModKey::Parameters& params, ModParam which, Args&&... args)
    {
        switch (which) {
        case ModParam::Depth: break;
        case ModParam::Curve: reply(params.curve); break;
        case ModParam::Smooth: reply(params.smooth); break;
        case ModParam::Step: reply(params.step, std::forward<Args>(args)...); break;
        }
    }

    template <class T, class... Args>
    void reply(const CCMap<T>& map, Args... args) { reply(map, true, args...); }

    template <class T, class... Args>
    void reply(const CCMap<T>& map, bool&& useDefault, Args... args)
    {
        if (useDefault)
            reply(map.getWithDefault(indices.back()), args...);
        else
            reply(map.get(indices.back()), args...);
    }

    template <class T, class... Args>
    void set(const CCMap<T>& map, Args... args)
    {
        set(map[indices.back()], args...);
    }

    template <class T, class... Args>
    void reply(const EGDescription& eg, T EGDescription::*&& member, Args&&... args)
    {
        reply(eg.*member, std::forward<Args>(args)...);
    }

    template <class T, class... Args>
    void set(EGDescription& eg, T EGDescription::*&& member, Args&&... args)
    {
        set(eg.*member, std::forward<Args>(args)...);
    }

    template <class T, class... Args>
    void set(CCMap<T>& map, Args&&... args) { set(map[indices.back()], std::forward<Args>(args)...); }

    // Below are all methods that will fetch various data structure elements (regions,
    // egs, ...), check that they exist, and call an actual "concrete" reply
    // implementation. They use pointer-to-member variables, which allow the compiler to
    // dispatch to the correct logic. For example, if the pointer-to-member is
    // `Region::*`, we aim at sending the value of a member of the Region
    // struct. The first thing is to read the index of the region and fetch it from the
    // sfizz data structure using `getRegion(...)`, and then apply the pointer-to-member
    // to this particular region to send the value.

    // Adding new dispatching overloads seemed simple enough to this point, although I
    // haven't found a nice way to the same with pointer-to-member function.
    // TODO: could do something with the c++14 compatible `invoke` maybe?

    template <class T, class M, class... Args>
    void set(T M::*member, Args&&... args)
    {
        dispatch(
            static_cast<void (MessagingHelper::*)(T&, Args && ...)>(&MessagingHelper::set),
            member,
            std::forward<Args>(args)...);
    }

    // MSVC require this because it finds T M::* and T Voice::* ambiguous in the overload
    // resolution of `reply`, so we "disable" the reply/set dispatching for Voice and
    // TriggerEvent since they're read-only components.
    template<class M>
    using DispatchedType = typename std::enable_if<
        !std::is_same<M, Voice>::value && !std::is_same<M, TriggerEvent>::value, void>::type;

    template <class T, class M, class... Args>
    DispatchedType<M> reply(T M::*member, Args&&... args)
    {
        dispatch(
            static_cast<void (MessagingHelper::*)(const T&, Args&&...)>(&MessagingHelper::reply),
            member,
            std::forward<Args>(args)...);
    }


    template <class T, class F, class... Args>
    void dispatch(F&& f, T FilterDescription::*member, Args&&... args)
    {
        if (auto region = getRegion())
            if (auto filter = getFilter(*region))
                inv::invoke(std::forward<F>(f), this, (*filter).*member, std::forward<Args>(args)...);
    }

    template <class T, class F, class... Args>
    void dispatch(F&& f, T SynthConfig::*member, Args&&... args)
    {
        inv::invoke(std::forward<F>(f), this, (impl.resources_.getSynthConfig()).*member, std::forward<Args>(args)...);
    }

    template <class T, class F, class... Args>
    void dispatch(F&& f, T LFODescription::Sub::*member, Args&&... args)
    {
        if (auto region = getRegion())
            if (auto lfo = getLFO(*region))
                if (auto sub = getLFOSub(*lfo))
                    inv::invoke(std::forward<F>(f), this, (*sub).*member, std::forward<Args>(args)...);
    }

    template <class T, class F, class... Args>
    void dispatch(F&& f, T Region::*member, Args&&... args)
    {
        if (auto region = getRegion())
            inv::invoke(std::forward<F>(f), this, (*region).*member, std::forward<Args>(args)...);
    }

    template <class T, class F, class... Args>
    void dispatch(F&& f, T EQDescription::*member, Args&&... args)
    {
        if (auto region = getRegion())
            if (auto eq = getEQ(*region))
                inv::invoke(std::forward<F>(f), this, (*eq).*member, std::forward<Args>(args)...);
    }

    template <class T, class F, class... Args>
    void dispatch(F&& f, T LFODescription::*member, Args&&... args)
    {
        if (auto region = getRegion())
            if (auto lfo = getLFO(*region))
                inv::invoke(std::forward<F>(f), this, (*lfo).*member, std::forward<Args>(args)...);
    }

    template <class T, class F, class... Args>
    void dispatch(F&& f, T FlexEGPoint::*member, Args&&... args)
    {
        if (auto region = getRegion())
            if (auto eg = getEG(*region))
                if (auto point = getEGPoint(*eg))
                    inv::invoke(std::forward<F>(f), this, (*point).*member, std::forward<Args>(args)...);
    }

    // No need to dispatch for voices, they are read-only for now

    template <class T, class... Args>
    void reply(T TriggerEvent::*member, Args&&... args)
    {
        if (auto voice = getVoice())
            reply((*voice).getTriggerEvent().*member, std::forward<Args>(args)...);
    }

    template <class T, class... Args>
    void reply(T Voice::*member, Args&&... args)
    {
        if (auto voice = getVoice()) {
            reply((*voice.*member)(std::forward<Args>(args)...)); // Voice only has callables
        }
    }

    template <class... Args>
    void reply(ModId id, ModParam param, Args&&... args)
    {
        if (auto region = getRegion()) {
            int cc = static_cast<int>(indices.back());
            switch (id) {
            case ModId::FilCutoff:
            case ModId::FilGain:
                switch (param) {
                case ModParam::Depth: reply(region->ccModDepth(cc, id, indices[1]), std::forward<Args>(args)...);
                    break;
                default:
                    reply(region->ccModParameters(cc, id, indices[1]), param, std::forward<Args>(args)...);
                    break;
                }
                break;
            default:
                switch (param) {
                case ModParam::Depth: reply(region->ccModDepth(cc, id), std::forward<Args>(args)...);
                    break;
                default:
                    reply(region->ccModParameters(cc, id), param, std::forward<Args>(args)...);
                    break;
                }
            }
        }
    }

    // Validate and fetch elements from the sfizz data structures. By default, we kind of
    // assume that regions/voices will be the first index, CCs will be the last, and
    // EQ/Filter/.. will be in-between.
    Region* getRegion(absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[0]);
        if (idx >= impl.layers_.size())
            return {};

        Layer& layer = *impl.layers_[idx];
        return &layer.getRegion();
    }

    FilterDescription* getFilter(Region& region, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[1]);
        if (region.filters.size() <= idx)
            return {};

        return &region.filters[idx];
    }

    EQDescription* getEQ(Region& region, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[1]);
        if (region.equalizers.size() <= idx)
            return {};

        return &region.equalizers[idx];
    }

    LFODescription* getLFO(Region& region, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[1]);
        if (region.lfos.size() <= idx)
            return {};

        return &region.lfos[idx];
    }

    LFODescription::Sub* getLFOSub(LFODescription& lfo, absl::optional<unsigned> index = {})
    {
        if (indices.size() == 2) {
            if (lfo.sub.empty())
                return {};
            else
                return &lfo.sub.front();
        }

        const auto idx = index.value_or(indices[2]);
        if (lfo.sub.size() <= idx)
            return {};

        return &lfo.sub[idx];
    }

    FlexEGDescription* getEG(Region& region, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[1]);
        if (region.flexEGs.size() <= idx)
            return {};

        return &region.flexEGs[idx];
    }

    FlexEGPoint* getEGPoint(FlexEGDescription& desc, absl::optional<unsigned> index = {})
    {
        const auto idx = index.value_or(indices[2]) + 1;
        if (desc.points.size() <= idx)
            return {};

        return &desc.points[idx];
    }

    Voice* getVoice(absl::optional<unsigned> index = {})
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
    template <class T = unsigned>
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
    Synth::Impl& impl;
    int delay;
    const char* path;
    const char* sig;
    const sfizz_arg_t* request_args;
};

}
