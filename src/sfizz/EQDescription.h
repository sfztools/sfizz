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
struct EQDescription
{
    float bandwidth { Default::eqBandwidth.value };
    float frequency { Default::eqFrequency.value };
    float gain { Default::eqGain.value };
    float vel2frequency { Default::eqVel2Frequency.value };
    float vel2gain { Default::eqVel2Gain.value };
    EqType type { EqType::kEqPeak };
};
}
