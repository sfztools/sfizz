// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

struct value_range {
    double min = 0.0;
    double max = 1.0;

    constexpr value_range()
    {
    }

    constexpr value_range(double min, double max)
        : min(min), max(max)
    {
    }

    double normalize(double x) const noexcept
    {
        return (x - min) / (max - min);
    }

    double denormalize(double x) const noexcept
    {
        return min + x * (max - min);
    }

    double clamp(double x) const noexcept
    {
        x = (x < min) ? min : x;
        x = (x > max) ? max : x;
        return x;
    }
};
