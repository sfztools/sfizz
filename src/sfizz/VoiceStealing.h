// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include "Config.h"
#include "Region.h"
#include "Voice.h"
#include "SisterVoiceRing.h"
#include <vector>
#include "absl/types/span.h"

namespace sfz
{

enum class StealingAlgorithm {
    First,
    Oldest,
    EnvelopeAndAge
};

class VoiceStealer
{
public:
    virtual ~VoiceStealer() {}
    /**
     * @brief Check that the region polyphony is respected.
     *
     * @param region
     * @param candidates
     * @param preferredChannel  if >= 0, the stealer prefers a victim
     *        whose triggering MIDI channel matches this value, and only
     *        falls back to a cross-channel victim if no same-channel
     *        candidate is suitable. -1 means "no preference" (default —
     *        preserves pre-MPE-fork stealing behavior).
     * @return Voice* a non-null voice if the region polyphony is not respected
     */
    virtual Voice* checkRegionPolyphony(const Region* region, absl::Span<Voice*> candidates, int preferredChannel = -1) = 0;
    /**
     * @brief Check that the polyphony is respected.
     *
     * @param candidates
     * @param maxPolyphony
     * @param preferredChannel  see checkRegionPolyphony.
     * @return Voice* a non-null voice if the polyphony is not respected
     */
    virtual Voice* checkPolyphony(absl::Span<Voice*> candidates, unsigned maxPolyphony, int preferredChannel = -1) = 0;
};

class FirstStealer final : public VoiceStealer
{
public:
    Voice* checkRegionPolyphony(const Region* region, absl::Span<Voice*> candidates, int preferredChannel = -1) final;
    Voice* checkPolyphony(absl::Span<Voice*> candidates, unsigned maxPolyphony, int preferredChannel = -1) final;
};

class OldestStealer final : public VoiceStealer
{
public:
    Voice* checkRegionPolyphony(const Region* region, absl::Span<Voice*> candidates, int preferredChannel = -1) final;
    Voice* checkPolyphony(absl::Span<Voice*> candidates, unsigned maxPolyphony, int preferredChannel = -1) final;
};

class EnvelopeAndAgeStealer final : public VoiceStealer
{
public:
    EnvelopeAndAgeStealer();
    Voice* checkRegionPolyphony(const Region* region, absl::Span<Voice*> candidates, int preferredChannel = -1) final;
    Voice* checkPolyphony(absl::Span<Voice*> candidates, unsigned maxPolyphony, int preferredChannel = -1) final;
private:
    std::vector<Voice*> temp_;
};

}
