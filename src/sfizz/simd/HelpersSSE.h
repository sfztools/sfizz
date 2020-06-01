// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

/* These are the SSE versions of the SIMDHelpers */
void readInterleavedSSE(const float* input, float* outputLeft, float* outputRight, unsigned inputSize) noexcept;
void writeInterleavedSSE(const float* inputLeft, const float* inputRight, float* output, unsigned outputSize) noexcept;
void applyGainSSE(float gain, const float* input, float* output, unsigned size) noexcept;
void applyGainSSE(const float* gain, const float* input, float* output, unsigned size) noexcept;
void divideSSE(const float* input, const float* divisor, float* output, unsigned size) noexcept;
void multiplyAddSSE(const float* gain, const float* input, float* output, unsigned size) noexcept;
void multiplyAddSSE(float gain, const float* input, float* output, unsigned size) noexcept;
float linearRampSSE(float* output, float start, float step, unsigned size) noexcept;
float multiplicativeRampSSE(float* output, float start, float step, unsigned size) noexcept;
void addSSE(const float* input, float* output, unsigned size) noexcept;
void addSSE(float value, float* output, unsigned size) noexcept;
void subtractSSE(const float* input, float* output, unsigned size) noexcept;
void subtractSSE(float value, float* output, unsigned size) noexcept;
void copySSE(const float* input, float* output, unsigned size) noexcept;
float meanSSE(const float* vector, unsigned size) noexcept;
float meanSquaredSSE(const float* vector, unsigned size) noexcept;
void cumsumSSE(const float* input, float* output, unsigned size) noexcept;
void diffSSE(const float* input, float* output, unsigned size) noexcept;
