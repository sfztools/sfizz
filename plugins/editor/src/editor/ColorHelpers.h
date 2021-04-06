// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "utility/vstgui_before.h"
#include "vstgui/lib/ccolor.h"
#include "utility/vstgui_after.h"
#include <absl/strings/string_view.h>

using namespace VSTGUI;

struct SColorRGB;
struct SColorHCY;

struct SColorRGB {
    SColorRGB() = default;
    explicit SColorRGB(const CColor &cc);
    explicit SColorRGB(const SColorHCY &hcy);
    SColorRGB(float r, float g, float b, float a = 1.0) : r(r), g(g), b(b), a(a) {}
    CColor toColor() const;

    float r {}, g {}, b {}, a { 1.0 };
};

struct SColorHCY {
    SColorHCY() = default;
    explicit SColorHCY(const CColor &cc) : SColorHCY(SColorRGB(cc)) {}
    explicit SColorHCY(const SColorRGB &rgb);
    SColorHCY(float h, float c, float y, float a = 1.0) : h(h), c(c), y(y), a(a) {}
    CColor toColor() const { return SColorRGB(*this).toColor(); }

    float h {}, c {}, y {}, a { 1.0 };
};

bool colorFromHex(absl::string_view hex, CColor& color);
