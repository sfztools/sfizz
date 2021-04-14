// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <simde/x86/sse.h>
#include <cstddef>
#include <cmath>

class RMSFollower {
public:
    RMSFollower()
    {
        updatePole();
    }

    void clear()
    {
        mem_ = simde_mm_setzero_ps();
    }

    void init(float sampleRate)
    {
        sampleRate_ = sampleRate;
        updatePole();
        clear();
    }

    void setT60(float t60)
    {
        t60_ = t60;
        updatePole();
    }

    void process(const float* leftBlock, const float* rightBlock, size_t numFrames)
    {
        simde__m128 mem = mem_;
        const simde__m128 pole = simde_mm_load1_ps(&pole_);
        for (size_t i = 0; i < numFrames; ++i) {
            float left = leftBlock[i];
            float right = rightBlock[i];
            simde__m128 input = simde_mm_setr_ps(left, right, 0.0f, 0.0f);
            input = simde_mm_mul_ps(input, input);
            simde__m128 output = simde_mm_add_ps(input, simde_mm_mul_ps(pole, simde_mm_sub_ps(mem, input)));
            mem = output;
        }
        mem_ = mem;
    }

    simde__m128 getMS() const
    {
        return mem_;
    }

    simde__m128 getRMS() const
    {
        return simde_mm_sqrt_ps(mem_);
    }

private:
    void updatePole()
    {
        pole_ = std::exp(float(-2.0 * M_PI) / (t60_ * sampleRate_));
    }

private:
    simde__m128 mem_ {};
    float pole_ {};
    float t60_ = 300e-3;
    float sampleRate_ = 44100;
};
