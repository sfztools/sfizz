// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "ColorHelpers.h"
#include <ColorSpaces.h>

SColorRGB::SColorRGB(const CColor &cc)
{
    r = cc.normRed<float>();
    g = cc.normGreen<float>();
    b = cc.normBlue<float>();
    a = cc.normAlpha<float>();
}

SColorRGB::SColorRGB(const SColorHCY &hcy)
{
    ColorSpaces::vec3 vhcy{{hcy.h, hcy.c, hcy.y}};
    ColorSpaces::vec3 vrgb = ColorSpaces::hcy_to_rgb(vhcy);
    r = vrgb[0];
    g = vrgb[1];
    b = vrgb[2];
    a = hcy.a;
}

CColor SColorRGB::toColor() const
{
    CColor cc;
    cc.setNormRed(r);
    cc.setNormGreen(g);
    cc.setNormBlue(b);
    cc.setNormAlpha(a);
    return cc;
}

SColorHCY::SColorHCY(const SColorRGB &rgb)
{
    ColorSpaces::vec3 vrgb{{rgb.r, rgb.g, rgb.b}};
    ColorSpaces::vec3 vhcy = ColorSpaces::rgb_to_hcy(vrgb);
    h = vhcy[0];
    c = vhcy[1];
    y = vhcy[2];
    a = rgb.a;
}

static int hexDigitFromChar(char c)
{
    return (c >= '0' && c <= '9') ? (c - '0') :
        (c >= 'a' && c <= 'z') ? (c - 'a' + 10) :
        (c >= 'A' && c <= 'Z') ? (c - 'A' + 10) : -1;
}

bool colorFromHex(absl::string_view hex, CColor& color)
{
    if (hex.empty() || hex[0] != '#')
        return false;

    hex = hex.substr(1, hex.size());
    size_t length = hex.size();
    uint32_t rgba = 0;
    if (length == 6 || length == 8) {
        for (size_t i = 0; i < length; ++i) {
            int d = hexDigitFromChar(hex[i]);
            if (d == -1)
                return false;

            rgba = (rgba << 4) | d;
        }
    }
    if (length == 6)
        rgba = (rgba << 8) | 0xff;

    color.red = rgba >> 24;
    color.green = (rgba >> 16) & 0xff;
    color.blue = (rgba >> 8) & 0xff;
    color.alpha = rgba & 0xff;

    return true;
}
