// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <absl/types/span.h>
#include <memory>
#include <complex>

namespace sfz {

/**
   A description of the harmonics of a particular wave form
 */
class HarmonicProfile {
public:
    virtual ~HarmonicProfile() {}

    static const HarmonicProfile& getSine();
    static const HarmonicProfile& getTriangle();
    static const HarmonicProfile& getSaw();
    static const HarmonicProfile& getSquare();

    /**
       @brief Get the value at the given index of the frequency spectrum.

       The modulus and the argument of the complex number are equal to the
       amplitude and the phase of the harmonic component.
     */
    virtual std::complex<double> getHarmonic(size_t index) const = 0;

    /**
       @brief Generate a period of the waveform and store it in the table.

       Do not generate harmonics above cutoff, which is expressed as Fc/Fs.
     */
    void generate(absl::Span<float> table, double amplitude, double cutoff) const;
};

/**
   A helper to select ranges of a multi-sampled oscillator, according to the
   frequency of an oscillator.

   The ranges are identified by octave numbers; not octaves in a musical sense,
   but as logarithmic divisions of the frequency range.
 */
class WavetableRange {
public:
    float minFrequency = 0;
    float maxFrequency = 0;

    static constexpr unsigned countOctaves = 10;
    static constexpr float frequencyScaleFactor = 0.05;

    static unsigned getOctaveForFrequency(float f);
    static WavetableRange getRangeForOctave(int o);
    static WavetableRange getRangeForFrequency(float f);

    // Note: using the frequency factor 0.05, octaves are as follows:
    //     octave 0: 20 Hz - 40 Hz
    //     octave 1: 40 Hz - 80 Hz
    //     octave 2: 80 Hz - 160 Hz
    //     octave 3: 160 Hz - 320 Hz
    //     octave 4: 320 Hz - 640 Hz
    //     octave 5: 640 Hz - 1280 Hz
    //     octave 6: 1280 Hz - 2560 Hz
    //     octave 7: 2560 Hz - 5120 Hz
    //     octave 8: 5120 Hz - 10240 Hz
    //     octave 9: 10240 Hz - 20480 Hz
};

/**
   Multisample of a wavetable, which is a collection of FFT-filtered mipmaps
   adapted for various playback frequencies.
 */
class WavetableMulti {
public:
    // number of tables in the multisample
    unsigned tableSize() const { return _tableSize; }

    // number of tables in the multisample
    static constexpr unsigned multiSize() { return WavetableRange::countOctaves; }

    // get the N-th table in the multisample
    absl::Span<const float> getTable(unsigned index) const
    {
        unsigned size = _tableSize;
        const float* ptr = &_multiData[index * _tableSize];
        return { ptr, size };
    }

    // get the table which is adequate for a given playback frequency
    absl::Span<const float> getTableForFrequency(float freq) const
    {
        return getTable(WavetableRange::getOctaveForFrequency(freq));
    }

    // create a multisample according to a given harmonic profile
    // the reference sample rate is the minimum value accepted by the DSP
    // system (most defavorable wrt. aliasing)
    static WavetableMulti createForHarmonicProfile(
        const HarmonicProfile& hp, unsigned tableSize, double refSampleRate = 44100.0);

private:
    // length of each individual table of the multisample
    unsigned _tableSize = 0;

    // internal storage, having `multiSize` rows and `tableSize` columns.
    std::unique_ptr<float[]> _multiData;
};

} // namespace sfz
