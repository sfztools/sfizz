#include "SIMDHelpers.h"
#include <array>

namespace sfz{

static std::array<bool, static_cast<unsigned>(sfz::SIMDOps::_sentinel)> simdStatus;
static bool simdStatusInitialized = false;

static void resetSIMDStatus()
{
    simdStatus[static_cast<unsigned>(SIMDOps::writeInterleaved)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::readInterleaved)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::fill)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::gain)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::divide)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::mathfuns)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::loopingSFZIndex)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::saturatingSFZIndex)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::linearRamp)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::multiplicativeRamp)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::add)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::subtract)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::multiplyAdd)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::copy)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::pan)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::cumsum)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::diff)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::sfzInterpolationCast)] = true;
    simdStatus[static_cast<unsigned>(SIMDOps::mean)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::meanSquared)] = false;
    simdStatus[static_cast<unsigned>(SIMDOps::upsampling)] = true;
    simdStatusInitialized = true;
}

static void setSIMDOpStatus(SIMDOps op, bool status)
{
    if (!simdStatusInitialized)
        resetSIMDStatus();

    simdStatus[static_cast<unsigned>(op)] = status;
}
static bool getSIMDOpStatus(SIMDOps op)
{
    if (!simdStatusInitialized)
        resetSIMDStatus();

    return simdStatus[static_cast<unsigned>(op)];
}

}

