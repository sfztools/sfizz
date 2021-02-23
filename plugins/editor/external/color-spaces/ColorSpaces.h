// SPDX-License-Identifier: MIT
/*
GLSL Color Space Utility Functions
(c) 2015 tobspr

Porting a subset to C++
(c) 2020 Jean Pierre Cimalando

-------------------------------------------------------------------------------

The MIT License (MIT)

Copyright (c) 2015

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

-------------------------------------------------------------------------------

Most formulars / matrices are from:
https://en.wikipedia.org/wiki/SRGB

Some are from:
http://www.chilliant.com/rgb2hsv.html
https://www.fourcc.org/fccyvrgb.php
*/

#pragma once
#include <array>
#include <algorithm>
#include <cmath>

namespace ColorSpaces {

template <std::size_t N> using vec = std::array<float, N>;
using vec3 = vec<3>;
using vec4 = vec<4>;

template <class T>
T clamp(T x, T lo, T hi)
{
    return std::max(lo, std::min(hi, x));
}

template <std::size_t N>
vec<N> saturate(vec<N> x)
{
    for (std::size_t i = 0; i < N; ++i)
        x[i] = clamp(x[i], 0.0f, 1.0f);
    return x;
}

float dot(vec3 a, vec3 b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Constants
static constexpr float HCV_EPSILON = 1e-10;
static constexpr float HCY_EPSILON = 1e-10;

// Converts a value from linear RGB to HCV (Hue, Chroma, Value)
vec3 rgb_to_hcv(vec3 rgb)
{
    // Based on work by Sam Hocevar and Emil Persson
    vec4 P = (rgb[1] < rgb[2]) ? vec4{{rgb[2], rgb[1], -1.0, 2.0/3.0}} : vec4{{rgb[1], rgb[2], 0.0, -1.0/3.0}};
    vec4 Q = (rgb[0] < P[0]) ? vec4{{P[0], P[1], P[3], rgb[0]}} : vec4{{rgb[0], P[1], P[2], P[0]}};
    float C = Q[0] - std::min(Q[3], Q[1]);
    float H = std::abs((Q[3] - Q[1]) / (6 * C + HCV_EPSILON) + Q[2]);
    return vec3{{H, C, Q[0]}};
}

// Converts from pure Hue to linear RGB
vec3 hue_to_rgb(float hue)
{
    float R = std::fabs(hue * 6 - 3) - 1;
    float G = 2 - std::fabs(hue * 6 - 2);
    float B = 2 - std::fabs(hue * 6 - 4);
    return saturate(vec3{{R,G,B}});
}

// Converts from HCY to linear RGB
vec3 hcy_to_rgb(vec3 hcy)
{
    const vec3 HCYwts{{0.299, 0.587, 0.114}};
    vec3 RGB = hue_to_rgb(hcy[0]);
    float Z = dot(RGB, HCYwts);
    if (hcy[2] < Z) {
        hcy[1] *= hcy[2] / Z;
    } else if (Z < 1) {
        hcy[1] *= (1 - hcy[2]) / (1 - Z);
    }
    return vec3{{(RGB[0] - Z)  * hcy[1] + hcy[2],
                 (RGB[1] - Z)  * hcy[1] + hcy[2],
                 (RGB[2] - Z)  * hcy[1] + hcy[2]}};
}

// Converts from rgb to hcy (Hue, Chroma, Luminance)
vec3 rgb_to_hcy(vec3 rgb)
{
    const vec3 HCYwts = vec3{{0.299, 0.587, 0.114}};
    // Corrected by David Schaeffer
    vec3 HCV = rgb_to_hcv(rgb);
    float Y = dot(rgb, HCYwts);
    float Z = dot(hue_to_rgb(HCV[0]), HCYwts);
    if (Y < Z) {
      HCV[1] *= Z / (HCY_EPSILON + Y);
    } else {
      HCV[1] *= (1 - Z) / (HCY_EPSILON + 1 - Y);
    }
    return vec3{{HCV[0], HCV[1], Y}};
}

} // namespace ColorSpaces
