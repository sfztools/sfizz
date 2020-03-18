// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
   Note(jpc): implementation status

   Complete (no opcodes)
*/

#include "Limiter.h"
#include "Opcode.h"
#include "gen/limiter.cpp"
#include "absl/memory/memory.h"

namespace sfz {
namespace fx {

    Limiter::Limiter()
        : _limiter(new faustLimiter)
    {
        _limiter->instanceResetUserInterface();
    }

    Limiter::~Limiter()
    {
    }

    void Limiter::setSampleRate(double sampleRate)
    {
        _limiter->classInit(sampleRate);
        _limiter->instanceConstants(sampleRate);
        clear();
    }

    void Limiter::setSamplesPerBlock(int samplesPerBlock)
    {
        (void)samplesPerBlock;
    }

    void Limiter::clear()
    {
        _limiter->instanceClear();
    }

    void Limiter::process(const float* const inputs[], float* const outputs[], unsigned nframes)
    {
        _limiter->compute(nframes, inputs, outputs);
    }

    std::unique_ptr<Effect> Limiter::makeInstance(absl::Span<const Opcode> members)
    {
        auto fx = absl::make_unique<Limiter>();

        for (const Opcode& opc : members) {
            // no opcodes
            (void)opc;
        }

        return CXX11_MOVE(fx);
    }

} // namespace fx
} // namespace sfz
