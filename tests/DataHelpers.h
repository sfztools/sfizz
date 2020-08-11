// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <ghc/fs_std.hpp>
#include <iosfwd>
#include <memory>
#include <cstddef>

struct DataPoints {
    size_t rows = 0;
    size_t cols = 0;
    std::unique_ptr<float[]> data;

    const float& operator()(size_t r, size_t c) const noexcept
    {
        return data[r * cols + c];
    }
    float& operator()(size_t r, size_t c) noexcept
    {
        return data[r * cols + c];
    }
};

void load_txt(DataPoints& dp, std::istream& in);
bool load_txt_file(DataPoints& dp, const fs::path& path);
