// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "Config.h"
#include "LeakDetector.h"
#include "Buffer.h"
#include <absl/types/span.h>
#include <memory>
#include <complex>

namespace sfz {

class WavetableMulti;

/**
   An oscillator based on wavetables
 */
class WavetableOscillator {
public:
    /**
       Initialize with the given sample rate.
       Run it once after instantiating.
     */
    void init(double sampleRate);

    /**
       Reset the oscillation to the initial phase.
     */
    void clear();

    /**
       Set the wavetable to generate with this oscillator.
     */
    void setWavetable(const WavetableMulti* wave);

    /**
       Compute a cycle of the oscillator, with constant frequency.
     */
    void process(float frequency, float* output, unsigned nframes);

    /**
       Compute a cycle of the oscillator, with varying frequency.
     */
    void processModulated(const float* frequencies, float* output, unsigned nframes);

private:
    /**
       Interpolate a value from a part of table, with delta in 0 to 1 excluded.
       There are `TableExtra` elements available for reading.
       (cf. WavetableMulti)
     */
    static float interpolate(const float* x, float delta);

private:
    float _phase = 0.0f;
    float _sampleInterval = 0.0f;
    const WavetableMulti* _multi = nullptr;
    LEAK_DETECTOR(WavetableOscillator);
};

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
    // number of elements in each table
    unsigned tableSize() const { return _tableSize; }

    // number of tables in the multisample
    static constexpr unsigned numTables() { return WavetableRange::countOctaves; }

    // get the N-th table in the multisample
    absl::Span<const float> getTable(unsigned index) const
    {
        return { getTablePointer(index), _tableSize };
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
        const HarmonicProfile& hp, double amplitude, unsigned tableSize = config::tableSize, double refSampleRate = 44100.0);

    // create the tiniest wavetable with null content for use with oscillators
    static WavetableMulti createSilence();

private:
    // get a pointer to the beginning of the N-th table
    const float* getTablePointer(unsigned index) const
    {
        return _multiData.data() + index * (_tableSize + _tableExtra);
    }

    // allocate the internal data for tables of the given size
    void allocateStorage(unsigned tableSize);

    // fill extra data at table ends with repetitions of the first samples
    void fillExtra();

    // length of each individual table of the multisample
    unsigned _tableSize = 0;

    // number X of extra elements, for safe interpolations up to X-th order.
    static constexpr unsigned _tableExtra = 4;

    // internal storage, having `multiSize` rows and `tableSize` columns.
    sfz::Buffer<float> _multiData;
    LEAK_DETECTOR(WavetableMulti);
};

/**
 * @brief Holds predefined wavetables.
 *
 */
struct WavetablePool {
    WavetablePool();
    static const WavetableMulti* getWaveSin();
    static const WavetableMulti* getWaveTriangle();
    static const WavetableMulti* getWaveSaw();
    static const WavetableMulti* getWaveSquare();
};

} // namespace sfz
