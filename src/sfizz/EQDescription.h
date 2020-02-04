#pragma once
#include "Config.h"
#include "Defaults.h"
#include "CCMap.h"

namespace sfz
{
struct EQDescription
{
    float bandwidth { Default::eqBandwidth };
    float frequency { Default::eqFrequencyUnset };
    float gain { Default::eqGain };
    float vel2frequency { Default::eqVel2frequency };
    float vel2gain { Default::eqVel2gain };
    CCMap<float> bandwidthCC { Default::eqBandwidthCC };
    CCMap<float> frequencyCC { Default::eqFrequencyCC };
    CCMap<float> gainCC { Default::eqGainCC };
};
}
