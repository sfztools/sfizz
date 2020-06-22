// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include "Config.h"
#include "Voice.h"
#include "SisterVoiceRing.h"
#include <vector>
#include "absl/types/span.h"

namespace sfz
{
class VoiceStealing
{
public:
    VoiceStealing()
    {
        voiceScores.reserve(config::maxVoices);
    }

    Voice* steal(absl::Span<Voice*> voices) noexcept
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

private:
    struct VoiceScore
    {
        Voice* voice;
        double score;
    };

    struct VoiceScoreComparator
    {
        bool operator()(const VoiceScore& voiceScore, const double& score)
        {
            return (voiceScore.score < score);
        }

        bool operator()(const double& score, const VoiceScore& voiceScore)
        {
            return (score < voiceScore.score);
        }

        bool operator()(const VoiceScore& lhs, const VoiceScore& rhs)
        {
            return (lhs.score < rhs.score);
        }
    };
    std::vector<VoiceScore> voiceScores;
};
}
