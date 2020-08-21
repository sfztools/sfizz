// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "../ModGenerator.h"

namespace sfz {
class Synth;

class LFOSource : public ModGenerator {
public:
    explicit LFOSource(Synth &synth);
    void init(const ModKey& sourceKey, NumericId<Voice> voiceId, unsigned delay) override;
    void generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer) override;

private:
    Synth* synth_ = nullptr;
};

} // namespace sfz
