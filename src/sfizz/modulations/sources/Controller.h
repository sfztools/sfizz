// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "../ModGenerator.h"
#include <memory>

namespace sfz {

struct Resources;

class ControllerSource : public ModGenerator {
public:
    explicit ControllerSource(Resources& res);
    ~ControllerSource();
    void setSampleRate(double sampleRate) override;
    void setSamplesPerBlock(unsigned count) override;
    void init(const ModKey& sourceKey, NumericId<Voice> voiceId) override;
    void generate(const ModKey& sourceKey, NumericId<Voice> voiceId, absl::Span<float> buffer) override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sfz
