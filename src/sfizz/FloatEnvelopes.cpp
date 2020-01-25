// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/**
 * @file FloatEnvelopes.cpp
 * @author Paul Ferrand (paul@ferrand.cc)
 * @brief Force the instantiations of the ADSR and linear envelopes for floats
 * @version 0.1
 * @date 2019-11-30
 *
 * @copyright Copyright (c) 2019 Paul Ferrand
 *
 */

#include "EventEnvelopes.h"
#include "ADSREnvelope.h"

// Include the generic implementations
#include "EventEnvelopes.cpp"
#include "ADSREnvelope.cpp"

// And explicitely instantiate the float version
namespace sfz
{
    template class EventEnvelope<float>;
    template class MultiplicativeEnvelope<float>;
    template class LinearEnvelope<float>;
    template class ADSREnvelope<float>;
}
