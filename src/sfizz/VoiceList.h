// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include "Voice.h"
#include "Config.h"
#include "Region.h"
#include "SisterVoiceRing.h"
#include "PolyphonyGroup.h"
#include "RegionSet.h"
#include "VoiceStealing.h"
#include <vector>
#include <absl/algorithm/container.h>

namespace sfz {

struct VoiceList : public Voice::StateListener
{
    /**
     * @brief The voice callback which is called during a change of state.
     */
    void onVoiceStateChanging(NumericId<Voice> id, Voice::State state) final
    {
        (void)id;
        if (state == Voice::State::idle) {
            auto voice = getVoiceById(id);
            RegionSet::removeVoiceFromHierarchy(voice->getRegion(), voice);
            swapAndPopFirst(activeVoices_, [voice](const Voice* v) { return v == voice; });
            polyphonyGroups_[voice->getRegion()->group].removeVoice(voice);
        } else if (state == Voice::State::playing) {
            auto voice = getVoiceById(id);
            activeVoices_.push_back(voice);
            RegionSet::registerVoiceInHierarchy(voice->getRegion(), voice);
            polyphonyGroups_[voice->getRegion()->group].registerVoice(voice);
        }
    }
    /**
     * @brief Find the voice which is associated with the given identifier.
     *
     * @param id
     * @return const Voice*
     */
    const Voice* getVoiceById(NumericId<Voice> id) const noexcept
    {
        const size_t size = list_.size();

        if (size == 0 || !id.valid())
            return nullptr;

        // search a sequence of ordered identifiers with potential gaps
        size_t index = static_cast<size_t>(id.number());
        index = std::min(index, size - 1);

        while (index > 0 && list_[index].getId().number() > id.number())
            --index;

        return (list_[index].getId() == id) ? &list_[index] : nullptr;
    }

    Voice* getVoiceById(NumericId<Voice> id) noexcept
    {
        return const_cast<Voice*>(
            const_cast<const VoiceList*>(this)->getVoiceById(id));
    }

    void reset()
    {
        for (auto& voice : list_)
            voice.reset();

        polyphonyGroups_.clear();
        polyphonyGroups_.emplace_back();
        polyphonyGroups_.back().setPolyphonyLimit(config::maxVoices);
        setStealingAlgorithm(StealingAlgorithm::Oldest);
    }

    bool playingAttackVoice(const Region* releaseRegion) noexcept
    {
        const auto compatibleVoice = [releaseRegion](const Voice& v) -> bool {
            const TriggerEvent& event = v.getTriggerEvent();
            return (
                !v.isFree()
                && event.type == TriggerEventType::NoteOn
                && releaseRegion->keyRange.containsWithEnd(event.number)
                && releaseRegion->velocityRange.containsWithEnd(event.value)
            );
        };

        if (absl::c_find_if(list_, compatibleVoice) == list_.end())
            return false;
        else
            return true;
    }

    void ensureNumPolyphonyGroups(unsigned groupIdx) noexcept
    {
        while (polyphonyGroups_.size() <= groupIdx)
            polyphonyGroups_.emplace_back();
    }

    void setGroupPolyphony(unsigned groupIdx, unsigned polyphony) noexcept
    {
        ensureNumPolyphonyGroups(groupIdx);
        polyphonyGroups_[groupIdx].setPolyphonyLimit(polyphony);
    }

    size_t getNumPolyphonyGroups() const noexcept { return polyphonyGroups_.size(); }

    const PolyphonyGroup* getPolyphonyGroupView(int idx) const noexcept
    {
        return (size_t)idx < polyphonyGroups_.size() ? &polyphonyGroups_[idx] : nullptr;
    }

    void clear()
    {
        reset();
        list_.clear();
        activeVoices_.clear();
    }

    void setStealingAlgorithm(StealingAlgorithm algorithm)
    {
        switch(algorithm){
        case StealingAlgorithm::First: // fallthrough
            for (auto& voice : list_)
                voice.disablePowerFollower();

            stealer_ = absl::make_unique<FirstStealer>();
            break;
        case StealingAlgorithm::Oldest:
            for (auto& voice : list_)
                voice.disablePowerFollower();

            stealer_ = absl::make_unique<OldestStealer>();
            break;
        case StealingAlgorithm::EnvelopeAndAge:
            for (auto& voice : list_)
                voice.enablePowerFollower();

            stealer_ = absl::make_unique<EnvelopeAndAgeStealer>();
            break;
        }
    }

    void checkPolyphony(const Region* region, int delay, const TriggerEvent& triggerEvent) noexcept
    {
        checkNotePolyphony(region, delay, triggerEvent);
        checkRegionPolyphony(region, delay);
        checkGroupPolyphony(region, delay);
        checkSetPolyphony(region, delay);
        checkEnginePolyphony(delay);
    }

    /**
     * @brief Get the number of active voices
     *
     * @return unsigned
     */
    unsigned getNumActiveVoices() const
    {
        return activeVoices_.size();
    }

    /**
     * @brief Find a voice that is not currently playing
     *
     * @return Voice*
     */
    Voice* findFreeVoice() noexcept
    {
        auto freeVoice = absl::c_find_if(list_, [](const Voice& voice) {
            return voice.isFree();
        });

        if (freeVoice != list_.end())
            return &*freeVoice;

        DBG("Engine hard polyphony reached");
        return {};
    }

private:
    std::vector<Voice> list_;
    std::vector<Voice*> activeVoices_;
    std::vector<Voice*> temp_;
    // These are the `group=` groups where you can off voices
    std::vector<PolyphonyGroup> polyphonyGroups_;
    std::unique_ptr<VoiceStealer> stealer_ { absl::make_unique<OldestStealer>() };

    /**
     * @brief Check the region polyphony, releasing voices if necessary
     *
     * @param region
     * @param delay
     */
    void checkRegionPolyphony(const Region* region, int delay) noexcept
    {
        Voice* candidate = stealer_->checkRegionPolyphony(region, absl::MakeSpan(activeVoices_));
        SisterVoiceRing::offAllSisters(candidate, delay);
    }

    /**
     * @brief Check the note polyphony, releasing voices if necessary
     *
     * @param region
     * @param delay
     * @param triggerEvent
     */
    void checkNotePolyphony(const Region* region, int delay, const TriggerEvent& triggerEvent) noexcept
    {
        if (!region->notePolyphony)
            return;

        unsigned notePolyphonyCounter { 0 };
        Voice* selfMaskCandidate { nullptr };

        for (Voice* voice : activeVoices_) {
            const TriggerEvent& voiceTriggerEvent = voice->getTriggerEvent();
            const bool skipVoice = (triggerEvent.type == TriggerEventType::NoteOn && voice->releasedOrFree()) || voice->isFree();
            if (!skipVoice
                && voice->getRegion()->group == region->group
                && voiceTriggerEvent.number == triggerEvent.number
                && voiceTriggerEvent.type == triggerEvent.type) {
                notePolyphonyCounter += 1;
                switch (region->selfMask) {
                case SfzSelfMask::mask:
                    if (voiceTriggerEvent.value <= triggerEvent.value) {
                        if (!selfMaskCandidate || selfMaskCandidate->getTriggerEvent().value > voiceTriggerEvent.value) {
                            selfMaskCandidate = voice;
                        }
                    }
                    break;
                case SfzSelfMask::dontMask:
                    if (!selfMaskCandidate || selfMaskCandidate->getAge() < voice->getAge())
                        selfMaskCandidate = voice;
                    break;
                }
            }
        }

        if (notePolyphonyCounter >= *region->notePolyphony) {
            SisterVoiceRing::offAllSisters(selfMaskCandidate, delay);
        }
    }

    /**
     * @brief Check the group polyphony, releasing voices if necessary
     *
     * @param region
     * @param delay
     */
    void checkGroupPolyphony(const Region* region, int delay) noexcept
    {
        auto& group = polyphonyGroups_[region->group];
        Voice* candidate = stealer_->checkPolyphony(
            absl::MakeSpan(group.getActiveVoices()), group.getPolyphonyLimit());
        SisterVoiceRing::offAllSisters(candidate, delay);
    }

    /**
     * @brief Check the region set polyphony at all levels, releasing voices if necessary
     *
     * @param region
     * @param delay
     */
    void checkSetPolyphony(const Region* region, int delay) noexcept
    {
        auto parent = region->parent;
        while (parent != nullptr) {
            Voice* candidate = stealer_->checkPolyphony(
                absl::MakeSpan(parent->getActiveVoices()), parent->getPolyphonyLimit());
            SisterVoiceRing::offAllSisters(candidate, delay);
            parent = parent->getParent();
        }
    }

    /**
     * @brief Check the engine polyphony, fast releasing voices if necessary
     *
     * @param delay
     */
    void checkEnginePolyphony(int delay) noexcept
    {
        // TODO (paul): should have the "required" vs "actual" number of voices here
        Voice* candidate = stealer_->checkPolyphony(
            absl::MakeSpan(activeVoices_), list_.size());
        SisterVoiceRing::offAllSisters(candidate, delay);
    }

public:
    // Vector shortcuts
    typename decltype(list_)::iterator begin() { return list_.begin(); }
    typename decltype(list_)::const_iterator cbegin() const { return list_.cbegin(); }
    typename decltype(list_)::iterator end() { return list_.end(); }
    typename decltype(list_)::const_iterator cend() const { return list_.cend(); }
    typename decltype(list_)::reference operator[] (size_t n) { return list_[n]; }
    typename decltype(list_)::const_reference operator[] (size_t n) const { return list_[n]; }
    typename decltype(list_)::reference back() { return list_.back(); }
    typename decltype(list_)::const_reference back() const { return list_.back(); }
    size_t size() const { return list_.size(); }
    void reserve(size_t n) { list_.reserve(n); }
    template< class... Args >
    void emplace_back(Args&&... args) { list_.emplace_back(std::forward<Args>(args)...); }
};

} // namespace sfz
