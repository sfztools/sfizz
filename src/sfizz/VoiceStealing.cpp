#include "VoiceStealing.h"

sfz::VoiceStealing::VoiceStealing()
{
    voiceScores.reserve(config::maxVoices);
}

sfz::Voice* sfz::VoiceStealing::steal(absl::Span<sfz::Voice*> voices) noexcept
{
    // Start of the voice stealing algorithm
    absl::c_sort(voices, voiceOrdering);

    const auto sumEnvelope = absl::c_accumulate(voices, 0.0f, [](float sum, const Voice* v) {
        return sum + v->getAverageEnvelope();
    });
    const auto envThreshold = sumEnvelope
        / static_cast<float>(voices.size()) * config::stealingEnvelopeCoeff;
    const auto ageThreshold = voices.front()->getAge() * config::stealingAgeCoeff;

    Voice* returnedVoice = voices.front();
    unsigned idx = 0;
    while (idx < voices.size()) {
        const auto ref = voices[idx];

        if (ref->getAge() < ageThreshold) {
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
