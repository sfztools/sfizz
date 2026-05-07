#include "VoiceStealing.h"
#include "SisterVoiceRing.h"

namespace sfz {

/**
 * @brief Generic polyphony checker
 * A voice is counted as incrementing the voice count and "stealable" if voiceCond(voice) is true.
 * For each stealable voice, the voice becomes the stealing candidate if candidateCond(voice, candidate) is true.
 *
 * When preferredChannel >= 0, two candidates are tracked: one across all
 * stealable voices, and one restricted to voices whose TriggerEvent.channel
 * matches preferredChannel. If the polyphony cap is exceeded and a
 * same-channel candidate exists, it is returned in preference to the
 * cross-channel candidate. preferredChannel = -1 reproduces the original
 * single-candidate behavior exactly.
 *
 * @tparam F
 * @tparam G
 * @param candidates
 * @param polyphony
 * @param voiceCond a functor with signature bool(Voice* voice)
 * @param candidateCond a functor with signature bool(Voice* voice, Voice* candidate)
 * @param preferredChannel see header
 * @return Voice*
 */
template<class F, class G>
Voice* genericPolyphonyCheck(absl::Span<Voice*> candidates, unsigned polyphony, F&& voiceCond, G&& candidateCond, int preferredChannel = -1)
{
    Voice* candidate = nullptr;
    Voice* sameChannelCandidate = nullptr;
    unsigned numPlaying = 0;
    for (const auto& voice : candidates) {
        if (voiceCond(voice)) {
            if (candidateCond(voice, candidate))
                candidate = voice;
            if (preferredChannel >= 0
                && voice != nullptr
                && voice->getTriggerEvent().channel == preferredChannel
                && candidateCond(voice, sameChannelCandidate)) {
                sameChannelCandidate = voice;
            }
            numPlaying += 1;
        }
    }

    if (numPlaying >= polyphony)
        return sameChannelCandidate ? sameChannelCandidate : candidate;

    return {};
}

/**
 * @brief Helper to ignore the voice if released depending on a boolean
 *
 * @param voice
 * @param ignoreReleased
 */
constexpr bool ignoreVoice(const Voice* voice)
{
    return (voice == nullptr || voice->offedOrFree());
}

Voice* FirstStealer::checkRegionPolyphony(const Region* region, absl::Span<Voice*> candidates, int preferredChannel)
{
    ASSERT(region);
    return genericPolyphonyCheck(candidates, region->polyphony,
        [=](const Voice* v) { return (!ignoreVoice(v) && v->getRegion() == region); },
        [=](const Voice*, const Voice* c) { return c == nullptr; },
        preferredChannel);
}

Voice* FirstStealer::checkPolyphony(absl::Span<Voice*> candidates, unsigned maxPolyphony, int preferredChannel)
{
    return genericPolyphonyCheck(candidates, maxPolyphony,
        [=](const Voice* v) { return (!ignoreVoice(v)); },
        [=](const Voice*, const Voice* c) { return c == nullptr; },
        preferredChannel);
}

Voice* OldestStealer::checkRegionPolyphony(const Region* region, absl::Span<Voice*> candidates, int preferredChannel)
{
    ASSERT(region);
    return genericPolyphonyCheck(candidates, region->polyphony,
        [=](const Voice* v) { return (!ignoreVoice(v) && v->getRegion() == region); },
        [=](const Voice* v, const Voice* c) { return (c == nullptr || v->getAge() > c->getAge()); },
        preferredChannel);
}

Voice* OldestStealer::checkPolyphony(absl::Span<Voice*> candidates, unsigned maxPolyphony, int preferredChannel)
{
    return genericPolyphonyCheck(candidates, maxPolyphony,
        [=](const Voice* v) { return (!ignoreVoice(v)); },
        [=](const Voice* v, const Voice* c) { return (c == nullptr || v->getAge() > c->getAge()); },
        preferredChannel);
}

/**
 * @brief Stealer on envelope and age.
 * The stealer checks that the power to try and kill voices with relative low contribution
 * to the output compared to the rest.
 * The stealer also checks the age so that voices have the time to build up attack
 * This is not perfect because pad-type voices will take a long time to output
 * their sound, but it's reasonable for sounds with a quick attack and longer
 * release.
 *
 * @param voices
 * @return sfz::Voice*
 */
sfz::Voice* stealEnvelopeAndAge(absl::Span<Voice*> voices) noexcept
{
    absl::c_sort(voices, voiceOrdering);

    const auto sumPower = absl::c_accumulate(voices, 0.0f, [](float sum, const Voice* v) {
        return sum + v->getAveragePower();
    });

    const auto powerThreshold = sumPower
        / static_cast<float>(voices.size()) * config::stealingPowerCoeff;
    const auto ageThreshold =
        static_cast<int>(voices.front()->getAge() * config::stealingAgeCoeff);

    Voice* returnedVoice = voices.front();
    unsigned idx = 0;
    while (idx < voices.size()) {
        const auto ref = voices[idx];

        if (ref->getAge() <= ageThreshold) {
            // Went too far, we'll kill the oldest note.
            break;
        }

        float maxPower { 0.0f };
        SisterVoiceRing::applyToRing(ref, [&](Voice* v) {
            maxPower = max(maxPower, v->getAveragePower());
        });

        if (maxPower < powerThreshold) {
            returnedVoice = ref;
            break;
        }

        // Jump over the sister voices in the set
        do { idx++; }
        while (idx < voices.size() && sisterVoices(ref, voices[idx]));
    }

    return returnedVoice;
}

/**
 * @brief Run the envelope-and-age scoring on a candidate set, optionally
 * preferring same-channel candidates first when preferredChannel >= 0.
 * Falls back to the full candidate set if the same-channel set is empty.
 */
static sfz::Voice* stealEnvelopeAndAgeWithChannelPref(
    std::vector<Voice*>& temp, int preferredChannel) noexcept
{
    if (preferredChannel >= 0) {
        std::vector<Voice*> sameChannel;
        sameChannel.reserve(temp.size());
        for (Voice* v : temp) {
            if (v != nullptr && v->getTriggerEvent().channel == preferredChannel)
                sameChannel.push_back(v);
        }
        if (!sameChannel.empty())
            return stealEnvelopeAndAge(absl::MakeSpan(sameChannel));
    }
    return stealEnvelopeAndAge(absl::MakeSpan(temp));
}

Voice* EnvelopeAndAgeStealer::checkRegionPolyphony(const Region* region, absl::Span<Voice*> candidates, int preferredChannel)
{
    ASSERT(region);
    temp_.clear();
    absl::c_copy_if(candidates, std::back_inserter(temp_), [=](Voice* v) {
            return (!ignoreVoice(v) && v->getRegion() == region);
    });

    if (temp_.size() >= region->polyphony)
        return stealEnvelopeAndAgeWithChannelPref(temp_, preferredChannel);

    return {};
}

Voice* EnvelopeAndAgeStealer::checkPolyphony(absl::Span<Voice*> candidates, unsigned maxPolyphony, int preferredChannel)
{
    temp_.clear();
    absl::c_copy_if(candidates, std::back_inserter(temp_), [=](Voice* v) {
            return !ignoreVoice(v);
    });

    if (temp_.size() >= maxPolyphony)
        return stealEnvelopeAndAgeWithChannelPref(temp_, preferredChannel);

    return {};
}

EnvelopeAndAgeStealer::EnvelopeAndAgeStealer()
{
    temp_.reserve(config::maxVoices);
}

}
