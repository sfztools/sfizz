// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <absl/types/span.h>
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

} // namespace sfz
