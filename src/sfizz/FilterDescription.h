// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Config.h"
#include "Defaults.h"
#include "SfzFilter.h"
#include "CCMap.h"

namespace sfz
{
struct FilterDescription
{
    float cutoff { Default::filterCutoff.value };
    float resonance { Default::filterCutoff.value };
    float gain { Default::filterGain.value };
    int keytrack { Default::filterKeytrack.value };
    uint8_t keycenter { Default::key.value };
    int veltrack { Default::filterVeltrack.value };
    float random { Default::filterRandom.value };
    FilterType type { FilterType::kFilterLpf2p };
};
}
