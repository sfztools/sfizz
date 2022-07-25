// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <simde/x86/sse.h>
#include <cstddef>
#include <cmath>
#ifdef _WIN32
#include <malloc.h>
#endif

class RMSFollower {
public:
    RMSFollower()
    {
        setNumOutputs(numOutputs_);
        updatePole();
    }

    ~RMSFollower()
    {
        freeAlignedMemory();
    }

    void clear()
    {
        for (int c = 0; c < numOutputs_; c += 4) {
            simde__m128* mem = (simde__m128*) (mem_ + c);
            *mem = simde_mm_setzero_ps();
        }
    }

    void setNumOutputs(int numOutputs)
    {
        freeAlignedMemory();
        numOutputs_ = numOutputs;
        memSize_ = numOutputs % 4 == 0 ? numOutputs * sizeof(float) : (numOutputs / 4 + 1) * 4 * sizeof(float);
#ifdef _WIN32
        mem_ = (float *)_aligned_malloc(memSize_, 4 * sizeof(float));
#elif __APPLE__
        mem_ = (float *)malloc(memSize_); // Should be 16 aligned
#else
        mem_ = (float *)aligned_alloc(4 * sizeof(float), memSize_);
#endif
        clear();
    }

    void init(float sampleRate)
    {
        sampleRate_ = sampleRate;
        updatePole();
    }

    void setT60(float t60)
    {
        t60_ = t60;
        updatePole();
    }

    void process(const float** blocks, size_t numFrames, size_t numChannels)
    {
        assert(numChannels <= static_cast<size_t>(numOutputs_));
        const auto numSafeChannels = (numChannels / 4) * 4;
        for (size_t c = 0; c < numSafeChannels; c += 4) {
            simde__m128* mem = (simde__m128*) (mem_ + c);
            process4(mem, &blocks[c], numFrames);
        }

        simde__m128* mem = (simde__m128*) (mem_ + numSafeChannels);
        const auto remainingChannels = numChannels - numSafeChannels;
        if (remainingChannels >= 4)
            process4(mem, &blocks[numSafeChannels], numFrames);
        else if (remainingChannels == 3)
            process3(mem, &blocks[numSafeChannels], numFrames);
        else if (remainingChannels == 2)
            process2(mem, &blocks[numSafeChannels], numFrames);
        else if (remainingChannels == 1)
            process1(mem, &blocks[numSafeChannels], numFrames);
    }

    void getMS(float* ms, size_t numChannels) const
    {
        assert(numChannels <= static_cast<size_t>(numOutputs_));
        const auto numSafeChannels = (numChannels / 4) * 4;
        for (size_t c = 0; c < numSafeChannels; c += 4) {
            simde__m128* mem = (simde__m128*) (mem_ + c);
            simde_mm_store_ps(ms + c, *mem);
        }

        simde__m128* mem = (simde__m128*) (mem_ + numSafeChannels);
        float* temp = (float*)&temp;
        simde_mm_store_ps(temp, *mem);
        for (size_t c = numSafeChannels, t = 0; c < numChannels; c++, t++)
            ms[c] = temp[t];
    }

    void getRMS(float* rms, size_t numChannels) const
    {
        assert(numChannels <= static_cast<size_t>(numOutputs_));
        const auto numSafeChannels = (numChannels / 4) * 4;
        for (size_t c = 0; c < numSafeChannels; c += 4) {
            simde__m128* mem = (simde__m128*) (mem_ + c);
            simde_mm_store_ps(rms + c, simde_mm_sqrt_ps(*mem));
        }

        simde__m128* mem = (simde__m128*) (mem_ + numSafeChannels);
        float* temp = (float*)&temp_;
        simde_mm_store_ps(temp, simde_mm_sqrt_ps(*mem));
        for (size_t c = numSafeChannels, t = 0; c < numChannels; c++, t++)
            rms[c] = temp[t];
    }

private:
    void freeAlignedMemory()
    {
        if (mem_)
#ifdef _WIN32
            _aligned_free(mem_);
#else
            free(mem_);
#endif
    }

    void updatePole()
    {
        pole_ = std::exp(float(-2.0 * M_PI) / (t60_ * sampleRate_));
    }

    void process1(simde__m128* mem, const float** blocks, size_t numFrames)
    {
        simde__m128 input;
        const simde__m128 pole = simde_mm_load1_ps(&pole_);
        for (size_t i = 0; i < numFrames; ++i) {
            input = simde_mm_setr_ps(
                blocks[0][i], 0.0f, 0.0f, 0.0f);
            input = simde_mm_mul_ps(input, input);
            *mem = simde_mm_add_ps(input, simde_mm_mul_ps(pole, simde_mm_sub_ps(*mem, input)));
        }
    }

    void process2(simde__m128* mem, const float** blocks, size_t numFrames)
    {
        simde__m128 input;
        const simde__m128 pole = simde_mm_load1_ps(&pole_);
        for (size_t i = 0; i < numFrames; ++i) {
            input = simde_mm_setr_ps(
                blocks[0][i], blocks[1][i], 0.0f, 0.0f);
            input = simde_mm_mul_ps(input, input);
            *mem = simde_mm_add_ps(input, simde_mm_mul_ps(pole, simde_mm_sub_ps(*mem, input)));
        }
    }

    void process3(simde__m128* mem, const float** blocks, size_t numFrames)
    {
        simde__m128 input;
        const simde__m128 pole = simde_mm_load1_ps(&pole_);
        for (size_t i = 0; i < numFrames; ++i) {
            input = simde_mm_setr_ps(
                blocks[0][i], blocks[1][i], blocks[2][i], 0.0f);
            input = simde_mm_mul_ps(input, input);
            *mem = simde_mm_add_ps(input, simde_mm_mul_ps(pole, simde_mm_sub_ps(*mem, input)));
        }
    }

    void process4(simde__m128* mem, const float** blocks, size_t numFrames)
    {
        simde__m128 input;
        const simde__m128 pole = simde_mm_load1_ps(&pole_);
        for (size_t i = 0; i < numFrames; ++i) {
            input = simde_mm_setr_ps(
                blocks[0][i], blocks[1][i], blocks[2][i], blocks[3][i]);
            input = simde_mm_mul_ps(input, input);
            *mem = simde_mm_add_ps(input, simde_mm_mul_ps(pole, simde_mm_sub_ps(*mem, input)));
        }
    }

private:
    float* mem_ { nullptr };
    simde__m128 temp_;
    float pole_ {};
    float t60_ = 300e-3;
    float sampleRate_ = 44100;
    int numOutputs_ = 2;
    int memSize_ = 4;
};
