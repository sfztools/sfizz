#include "VoiceStealing.h"

sfz::VoiceStealing::VoiceStealing()
{
    voiceScores.reserve(config::maxVoices);
}

sfz::Voice* sfz::VoiceStealing::steal(absl::Span<sfz::Voice*> voices) noexcept
{
    if (voices.empty())
        return {};

    // Start of the voice stealing algorithm
    absl::c_stable_sort(voices, voiceOrdering);

    const auto sumEnvelope = absl::c_accumulate(voices, 0.0f, [](float sum, const Voice* v) {
        return sum + v->getAverageEnvelope();
    });
    // We are checking the envelope to try and kill voices with relative low contribution
    // to the output compared to the rest.
    const auto envThreshold = sumEnvelope
        / static_cast<float>(voices.size()) * config::stealingEnvelopeCoeff;
    // We are checking the age so that voices have the time to build up attack
    // This is not perfect because pad-type voices will take a long time to output
    // their sound, but it's reasonable for sounds with a quick attack and longer
    // release.
    const auto ageThreshold =
        static_cast<int>(voices.front()->getAge() * config::stealingAgeCoeff) + 1;

    Voice* returnedVoice = voices.front();
    unsigned idx = 0;
    while (idx < voices.size()) {
        const auto ref = voices[idx];

        if (ref->getAge() <= ageThreshold) {
            // Went too far, we'll kill the oldest note.
            break;
        }

        float maxEnvelope { 0.0f };
        SisterVoiceRing::applyToRing(ref, [&](Voice* v) {
            maxEnvelope = max(maxEnvelope, v->getAverageEnvelope());
        });

        if (maxEnvelope < envThreshold) {
            returnedVoice = ref;
            break;
        }

        // Jump over the sister voices in the set
        do { idx++; }
        while (idx < voices.size() && sisterVoices(ref, voices[idx]));
    }

    return returnedVoice;
}
