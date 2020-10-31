// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include "Voice.h"
#include "Region.h"
#include <vector>
#include <absl/algorithm/container.h>

namespace sfz {

struct VoiceList
{
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

    typename std::vector<Voice>::iterator begin() { return list_.begin(); }
    typename std::vector<Voice>::const_iterator cbegin() const { return list_.cbegin(); }
    typename std::vector<Voice>::iterator end() { return list_.end(); }
    typename std::vector<Voice>::const_iterator cend() const { return list_.cend(); }
    typename std::vector<Voice>::reference operator[] (size_t n) { return list_[n]; }
    typename std::vector<Voice>::const_reference operator[] (size_t n) const { return list_[n]; }
    typename std::vector<Voice>::reference back() { return list_.back(); }
    typename std::vector<Voice>::const_reference back() const { return list_.back(); }
    size_t size() const { return list_.size(); }
    void clear() { list_.clear(); }
    void reserve(size_t n) { list_.reserve(n); }
    template< class... Args >
    void emplace_back(Args&&... args) { list_.emplace_back(std::forward<Args>(args)...); }
private:
    std::vector<Voice> list_;
};

} // namespace sfz
