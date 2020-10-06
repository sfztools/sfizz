#include "VoiceStealing.h"

sfz::VoiceStealing::VoiceStealing()
{
    voiceScores.reserve(config::maxVoices);
}

sfz::Voice* sfz::VoiceStealing::steal(absl::Span<sfz::Voice*> voices) noexcept
{
    if (voices.empty())
        return {};

    switch(stealingAlgorithm) {
    case StealingAlgorithm::First:
        return stealFirst(voices);
    case StealingAlgorithm::EnvelopeAndAge:
        return stealEnvelopeAndAge(voices);
    case StealingAlgorithm::Oldest:
    default:
        return stealOldest(voices);
    }
}

void sfz::VoiceStealing::setStealingAlgorithm(StealingAlgorithm algorithm) noexcept
{
    stealingAlgorithm = algorithm;
}

sfz::Voice* sfz::VoiceStealing::stealFirst(absl::Span<Voice*> voices) noexcept
{
    return voices.front();
}

sfz::Voice* sfz::VoiceStealing::stealOldest(absl::Span<Voice*> voices) noexcept
{
    absl::c_stable_sort(voices, voiceOrdering);
    return voices.front();
}

sfz::Voice* sfz::VoiceStealing::stealEnvelopeAndAge(absl::Span<Voice*> voices) noexcept
{
    absl::c_stable_sort(voices, voiceOrdering);

    const auto sumPower = absl::c_accumulate(voices, 0.0f, [](float sum, const Voice* v) {
        return sum + v->getAveragePower();
    });
    // We are checking the power to try and kill voices with relative low contribution
    // to the output compared to the rest.
    const auto powerThreshold = sumPower
        / static_cast<float>(voices.size()) * config::stealingPowerCoeff;
    // We are checking the age so that voices have the time to build up attack
    // This is not perfect because pad-type voices will take a long time to output
    // their sound, but it's reasonable for sounds with a quick attack and longer
    // release.
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
