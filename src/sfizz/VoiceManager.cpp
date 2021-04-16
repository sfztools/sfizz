// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "VoiceManager.h"
#include "SisterVoiceRing.h"
#include "RegionSet.h"
#include <absl/algorithm/container.h>

namespace sfz {

void VoiceManager::onVoiceStateChanging(NumericId<Voice> id, Voice::State state)
{
    if (state == Voice::State::idle) {
        Voice* voice = getVoiceById(id);
        const Region* region = voice->getRegion();
        const uint32_t group = region->group;
        RegionSet::removeVoiceFromHierarchy(region, voice);
        swapAndPopFirst(activeVoices_, [voice](const Voice* v) { return v == voice; });
        ASSERT(group < polyphonyGroups_.size());
        polyphonyGroups_[group].removeVoice(voice);
    } else if (state == Voice::State::playing) {
        Voice* voice = getVoiceById(id);
        const Region* region = voice->getRegion();
        const uint32_t group = region->group;
        activeVoices_.push_back(voice);
        RegionSet::registerVoiceInHierarchy(region, voice);
        ASSERT(group < polyphonyGroups_.size());
        polyphonyGroups_[group].registerVoice(voice);
    }
}

const Voice* VoiceManager::getVoiceById(NumericId<Voice> id) const noexcept
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

Voice* VoiceManager::getVoiceById(NumericId<Voice> id) noexcept
{
    return const_cast<Voice*>(
        const_cast<const VoiceManager*>(this)->getVoiceById(id));
}

void VoiceManager::reset()
{
    for (auto& voice : list_)
        voice.reset();

    polyphonyGroups_.clear();
    polyphonyGroups_.emplace_back();
    polyphonyGroups_.back().setPolyphonyLimit(config::maxVoices);
    setStealingAlgorithm(StealingAlgorithm::Oldest);
}

bool VoiceManager::playingAttackVoice(const Region* releaseRegion) noexcept
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

void VoiceManager::ensureNumPolyphonyGroups(unsigned groupIdx) noexcept
{
    size_t neededSize = static_cast<size_t>(groupIdx) + 1;
    if (polyphonyGroups_.size() < neededSize)
        polyphonyGroups_.resize(neededSize);
}

void VoiceManager::setGroupPolyphony(unsigned groupIdx, unsigned polyphony) noexcept
{
    ensureNumPolyphonyGroups(groupIdx);
    polyphonyGroups_[groupIdx].setPolyphonyLimit(polyphony);
}


const PolyphonyGroup* VoiceManager::getPolyphonyGroupView(int idx) const noexcept
{
    return (size_t)idx < polyphonyGroups_.size() ? &polyphonyGroups_[idx] : nullptr;
}

void VoiceManager::clear()
{
    for (PolyphonyGroup& pg : polyphonyGroups_)
        pg.removeAllVoices();
    list_.clear();
    activeVoices_.clear();
}

void VoiceManager::setStealingAlgorithm(StealingAlgorithm algorithm)
{
    switch(algorithm){
    case StealingAlgorithm::First:
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

void VoiceManager::checkPolyphony(const Region* region, int delay, const TriggerEvent& triggerEvent) noexcept
{
    checkNotePolyphony(region, delay, triggerEvent);
    checkRegionPolyphony(region, delay);
    checkGroupPolyphony(region, delay);
    checkSetPolyphony(region, delay);
    checkEnginePolyphony(delay);
}

Voice* VoiceManager::findFreeVoice() noexcept
{
    auto freeVoice = absl::c_find_if(list_, [](const Voice& voice) {
        return voice.isFree();
    });

    if (freeVoice != list_.end())
        return &*freeVoice;

    DBG("Engine hard polyphony reached");
    return {};
}

void VoiceManager::requireNumVoices(int numVoices, Resources& resources)
{
    numRequiredVoices_ = numVoices;
    const int numEffectiveVoices = getNumEffectiveVoices();

    clear();
    list_.reserve(numEffectiveVoices);
    activeVoices_.reserve(numEffectiveVoices);

    for (int i = 0; i < numEffectiveVoices; ++i) {
        list_.emplace_back(i, resources);
        Voice& lastVoice = list_.back();
        lastVoice.setStateListener(this);
    }
}

void VoiceManager::checkRegionPolyphony(const Region* region, int delay) noexcept
{
    Voice* candidate = stealer_->checkRegionPolyphony(region, absl::MakeSpan(activeVoices_));
    SisterVoiceRing::offAllSisters(candidate, delay);
}

void VoiceManager::checkNotePolyphony(const Region* region, int delay, const TriggerEvent& triggerEvent) noexcept
{
    if (!region->notePolyphony)
        return;

    unsigned notePolyphonyCounter { 0 };
    Voice* selfMaskCandidate { nullptr };

    for (Voice* voice : activeVoices_) {
        const TriggerEvent& voiceTriggerEvent = voice->getTriggerEvent();
        if (!voice->releasedOrFree()
            && voice->getRegion()->group == region->group
            && voiceTriggerEvent.number == triggerEvent.number) {
            notePolyphonyCounter += 1;
            switch (region->selfMask) {
            case SelfMask::mask:
                if (voiceTriggerEvent.value <= triggerEvent.value) {
                    if (!selfMaskCandidate
                        || selfMaskCandidate->getTriggerEvent().value > voiceTriggerEvent.value) {
                        selfMaskCandidate = voice;
                    }
                }
                break;
            case SelfMask::dontMask:
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

void VoiceManager::checkGroupPolyphony(const Region* region, int delay) noexcept
{
    auto& group = polyphonyGroups_[region->group];
    Voice* candidate = stealer_->checkPolyphony(
        absl::MakeSpan(group.getActiveVoices()), group.getPolyphonyLimit());
    SisterVoiceRing::offAllSisters(candidate, delay);
}

void VoiceManager::checkSetPolyphony(const Region* region, int delay) noexcept
{
    auto parent = region->parent;
    while (parent != nullptr) {
        Voice* candidate = stealer_->checkPolyphony(
            absl::MakeSpan(parent->getActiveVoices()), parent->getPolyphonyLimit());
        SisterVoiceRing::offAllSisters(candidate, delay);
        parent = parent->getParent();
    }
}

void VoiceManager::checkEnginePolyphony(int delay) noexcept
{
    Voice* candidate = stealer_->checkPolyphony(
        absl::MakeSpan(activeVoices_), numRequiredVoices_);
    SisterVoiceRing::offAllSisters(candidate, delay);
}

} // namespace sfz
