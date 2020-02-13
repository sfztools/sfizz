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

} // namespace sfz
