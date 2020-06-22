// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz
#pragma once

#include "Voice.h"
#include "absl/meta/type_traits.h"

namespace sfz
{

struct SisterVoiceRing {
    template<class F, class T,
        absl::enable_if_t<std::is_same<Voice, absl::remove_const_t<T>>::value, int> = 0>
    static void applyToRing(T* voice, F&& lambda) noexcept
    {
        auto v = voice->getNextSisterVoice();
        while (v != voice) {
            const auto next = v->getNextSisterVoice();
            lambda(v);
            v = next;
        }
        lambda(voice);
    }

    static unsigned countSisterVoices(const Voice* start) noexcept
    {
        if (!start)
            return 0;

        unsigned count = 0;
        auto next = start;
        do
        {
            count++;
            next = next->getNextSisterVoice();
        } while (next != start && count < config::maxVoices);

        ASSERT(count < config::maxVoices);
        return count;
    }
};

/**
 * @brief RAII helper to build sister voice rings.
 * Closes the doubly-linked list on destruction.
 *
 */
class SisterVoiceRingBuilder {
public:
    ~SisterVoiceRingBuilder() noexcept {
        if (lastStartedVoice != nullptr) {
            ASSERT(firstStartedVoice);
            lastStartedVoice->setNextSisterVoice(firstStartedVoice);
            firstStartedVoice->setPreviousSisterVoice(lastStartedVoice);
        }
    }

    /**
     * @brief Add a voice to the sister ring
     *
     * @param voice
     */
    void addVoiceToRing(Voice* voice) noexcept {
        if (firstStartedVoice == nullptr)
            firstStartedVoice = voice;

        if (lastStartedVoice != nullptr) {
            voice->setPreviousSisterVoice(lastStartedVoice);
            lastStartedVoice->setNextSisterVoice(voice);
        }

        lastStartedVoice = voice;
    }
    /**
     * @brief Apply a function to the sister ring, including the current voice.
     * This function should be safe enough to even reset the sister voices, but
     * if you mutate the ring significantly you should probably roll your own
     * iterator.
     *
     * @param lambda the function to apply.
     * @param voice the starting voice
     */
private:
    Voice* firstStartedVoice { nullptr };
    Voice* lastStartedVoice { nullptr };
};

}
