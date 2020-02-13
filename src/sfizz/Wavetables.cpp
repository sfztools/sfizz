// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Wavetables.h"
#include "MathHelpers.h"
#include <kiss_fftr.h>
#include <memory>

namespace sfz {

void HarmonicProfile::generate(
    absl::Span<float> table, double amplitude, double cutoff) const
{
    size_t size = table.size();

    typedef std::complex<kiss_fft_scalar> cpx;

    // allocate a spectrum of size N/2+1
    // bins are equispaced in frequency, with index N/2 being nyquist
    std::unique_ptr<cpx[]> spec(new cpx[size / 2 + 1]());

    kiss_fftr_cfg cfg = kiss_fftr_alloc(size, true, nullptr, nullptr);
    if (!cfg)
        throw std::bad_alloc();

    // bins need scaling and phase offset; this IFFT is a sum of cosines
    const std::complex<double> k = std::polar(amplitude * 0.5, M_PI / 2);

    // start filling at bin index 1; 1 is fundamental, 0 is DC
    for (size_t index = 1; index < size / 2 + 1; ++index) {
        if (index * (1.0 / size) > cutoff)
            break;

        std::complex<double> harmonic = getHarmonic(index);
        spec[index] = k * harmonic;
    }

    kiss_fftri(cfg, reinterpret_cast<kiss_fft_cpx*>(spec.get()), table.data());
    kiss_fftr_free(cfg);
}

class SineProfile : public HarmonicProfile {
public:
    std::complex<double> getHarmonic(size_t index) const
    {
        return (index == 1) ? 1.0 : 0.0;
    }
};

class TriangleProfile : public HarmonicProfile {
public:
    std::complex<double> getHarmonic(size_t index) const
    {
        if ((index & 1) == 0)
            return 0.0;

        bool s = (index >> 1) & 1;
        return std::polar<double>(
            (8 / (M_PI * M_PI)) * (1.0 / (index * index)),
            s ? 0.0 : M_PI);
    }
};

class SawProfile : public HarmonicProfile {
public:
    std::complex<double> getHarmonic(size_t index) const
    {
        if (index < 1)
            return 0.0;

        return std::polar(
            (2.0 / M_PI) / index,
            (index & 1) ? 0.0 : M_PI);
    }
};

class SquareProfile : public HarmonicProfile {
public:
    std::complex<double> getHarmonic(size_t index) const
    {
        if ((index & 1) == 0)
            return 0.0;

        return std::polar((4.0 / M_PI) / index, M_PI);
    }
};

static const SineProfile sineProfile;
static const TriangleProfile triangleProfile;
static const SawProfile sawProfile;
static const SquareProfile squareProfile;

const HarmonicProfile& HarmonicProfile::getSine()
{
    return sineProfile;
}

const HarmonicProfile& HarmonicProfile::getTriangle()
{
    return triangleProfile;
}

const HarmonicProfile& HarmonicProfile::getSaw()
{
    return sawProfile;
}

const HarmonicProfile& HarmonicProfile::getSquare()
{
    return squareProfile;
}

//------------------------------------------------------------------------------
constexpr unsigned WavetableRange::countOctaves;
constexpr float WavetableRange::frequencyScaleFactor;

unsigned WavetableRange::getOctaveForFrequency(float f)
{
    int oct = fp_exponent(frequencyScaleFactor * f);
    return clamp<int>(oct, 0, countOctaves - 1);
}

WavetableRange WavetableRange::getRangeForOctave(int o)
{
    WavetableRange range;

    Fraction<uint64_t> mant = fp_mantissa(0.0f);
    float k = 1.0f / frequencyScaleFactor;

    range.minFrequency = k * fp_from_parts<float>(0, o, 0);
    range.maxFrequency = k * fp_from_parts<float>(0, o, mant.den - 1);

    return range;
}

WavetableRange WavetableRange::getRangeForFrequency(float f)
{
    int oct = getOctaveForFrequency(f);
    return getRangeForOctave(oct);
}

} // namespace sfz
