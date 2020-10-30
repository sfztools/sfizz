// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include "Voice.h"
#include <vector>

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
        const size_t size = list.size();

        if (size == 0 || !id.valid())
            return nullptr;

        // search a sequence of ordered identifiers with potential gaps
        size_t index = static_cast<size_t>(id.number());
        index = std::min(index, size - 1);

        while (index > 0 && list[index].getId().number() > id.number())
            --index;

        return (list[index].getId() == id) ? &list[index] : nullptr;
    }

    Voice* getVoiceById(NumericId<Voice> id) noexcept
    {
        return const_cast<Voice*>(
            const_cast<const VoiceList*>(this)->getVoiceById(id));
    }

    void reset()
    {
        for (auto& voice : list)
            voice.reset();
    }

    typename std::vector<Voice>::iterator begin() { return list.begin(); }
    typename std::vector<Voice>::const_iterator cbegin() const { return list.cbegin(); }
    typename std::vector<Voice>::iterator end() { return list.end(); }
    typename std::vector<Voice>::const_iterator cend() const { return list.cend(); }
    typename std::vector<Voice>::reference operator[] (size_t n) { return list[n]; }
    typename std::vector<Voice>::const_reference operator[] (size_t n) const { return list[n]; }
    typename std::vector<Voice>::reference back() { return list.back(); }
    typename std::vector<Voice>::const_reference back() const { return list.back(); }
    size_t size() const { return list.size(); }
    void clear() { list.clear(); }
    void reserve(size_t n) { list.reserve(n); }
    template< class... Args >
    void emplace_back(Args&&... args) { list.emplace_back(std::forward<Args>(args)...); }
private:
    std::vector<Voice> list;
};

} // namespace sfz
