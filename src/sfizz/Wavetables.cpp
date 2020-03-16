// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Wavetables.h"
#include "FilePool.h"
#include "MathHelpers.h"
#include <kiss_fftr.h>

namespace sfz {

static WavetableMulti silenceMulti = WavetableMulti::createSilence();

void WavetableOscillator::init(double sampleRate)
{
    _sampleInterval = 1.0 / sampleRate;
    _multi = &silenceMulti;
    clear();
}

void WavetableOscillator::clear()
{
    _phase = 0.0f;
}

void WavetableOscillator::setWavetable(const WavetableMulti* wave)
{
    _multi = wave ? wave : &silenceMulti;
}

void WavetableOscillator::setPhase(float phase)
{
    ASSERT(phase >= 0.0f && phase <= 1.0f);
    _phase = phase;
}

void WavetableOscillator::process(float frequency, float* output, unsigned nframes)
{
    float phase = _phase;
    float phaseInc = frequency * _sampleInterval;

    const WavetableMulti& multi = *_multi;
    unsigned tableSize = multi.tableSize();
    absl::Span<const float> table = multi.getTableForFrequency(frequency);

    for (unsigned i = 0; i < nframes; ++i) {
        float position = phase * tableSize;
        unsigned index = static_cast<unsigned>(position);
        float frac = position - index;
        output[i] = interpolate(&table[index], frac);

        phase += phaseInc;
        phase -= static_cast<int>(phase);
    }

    _phase = phase;
}

void WavetableOscillator::processModulated(const float* frequencies, float* output, unsigned nframes)
{
    float phase = _phase;
    float sampleInterval = _sampleInterval;

    const WavetableMulti& multi = *_multi;
    unsigned tableSize = multi.tableSize();

    for (unsigned i = 0; i < nframes; ++i) {
        float frequency = frequencies[i];
        float phaseInc = frequency * sampleInterval;
        absl::Span<const float> table = multi.getTableForFrequency(frequency);

        float position = phase * tableSize;
        unsigned index = static_cast<unsigned>(position);
        float frac = position - index;
        output[i] = interpolate(&table[index], frac);

        phase += phaseInc;
        phase -= static_cast<int>(phase);
    }

    _phase = phase;
}

float WavetableOscillator::interpolate(const float* x, float delta)
{
    return x[0] + delta * (x[1] - x[0]);
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
constexpr unsigned WavetableMulti::_tableExtra;

WavetableMulti WavetableMulti::createForHarmonicProfile(
    const HarmonicProfile& hp, double amplitude, unsigned tableSize, double refSampleRate)
{
    WavetableMulti wm;
    constexpr unsigned numTables = WavetableMulti::numTables();

    wm.allocateStorage(tableSize);

    for (unsigned m = 0; m < numTables; ++m) {
        WavetableRange range = WavetableRange::getRangeForOctave(m);

        double freq = range.maxFrequency;

        // A spectrum S of fundamental F has: S[1]=F and S[N/2]=Fs'/2
        // which lets it generate frequency up to Fs'/2=F*N/2.
        // Therefore it's desired to cut harmonics at C=0.5*Fs/Fs'=0.5*Fs/(F*N).
        double cutoff = (0.5 * refSampleRate / tableSize) / freq;

        float* ptr = const_cast<float*>(wm.getTablePointer(m));
        absl::Span<float> table(ptr, tableSize);

        hp.generate(table, amplitude, cutoff);
    }

    wm.fillExtra();

    return wm;
}

WavetableMulti WavetableMulti::createSilence()
{
    WavetableMulti wm;
    wm.allocateStorage(1);
    wm.fillExtra();
    return wm;
}

void WavetableMulti::allocateStorage(unsigned tableSize)
{
    _multiData.resize((tableSize + _tableExtra) * numTables());
    _tableSize = tableSize;
}

void WavetableMulti::fillExtra()
{
    unsigned tableSize = _tableSize;
    constexpr unsigned tableExtra = _tableExtra;
    constexpr unsigned numTables = WavetableMulti::numTables();

    for (unsigned m = 0; m < numTables; ++m) {
        float* ptr = const_cast<float*>(getTablePointer(m));
        for (unsigned i = 0; i < tableExtra; ++i)
            ptr[tableSize + i] = ptr[i % tableSize];
    }
}

//------------------------------------------------------------------------------

/**
 * @brief Harmonic profile which takes its values from a table.
 */
class TabulatedHarmonicProfile : public HarmonicProfile {
public:
    explicit TabulatedHarmonicProfile(absl::Span<const std::complex<float>> harmonics)
        : _harmonics(harmonics)
    {
    }

    std::complex<double> getHarmonic(size_t index) const override
    {
        if (index >= _harmonics.size())
            return {};

        return _harmonics[index];
    }

private:
    absl::Span<const std::complex<float>> _harmonics;
};

//------------------------------------------------------------------------------

WavetablePool::WavetablePool()
{
    getWaveSin();
    getWaveTriangle();
    getWaveSaw();
    getWaveSquare();
}

const WavetableMulti* WavetablePool::getWaveSin()
{
    static auto wave = WavetableMulti::createForHarmonicProfile(
            HarmonicProfile::getSine(), config::amplitudeSine);
    return &wave;
}
const WavetableMulti* WavetablePool::getWaveTriangle()
{
    static auto wave = WavetableMulti::createForHarmonicProfile(
            HarmonicProfile::getTriangle(), config::amplitudeTriangle);
    return &wave;
}
const WavetableMulti* WavetablePool::getWaveSaw()
{
    static auto wave = WavetableMulti::createForHarmonicProfile(
            HarmonicProfile::getSaw(), config::amplitudeSaw);
    return &wave;
}
const WavetableMulti* WavetablePool::getWaveSquare()
{
    static auto wave = WavetableMulti::createForHarmonicProfile(
            HarmonicProfile::getSquare(), config::amplitudeSquare);
    return &wave;
}

const WavetableMulti* WavetablePool::getFileWave(const std::string& filename)
{
    auto it = _fileWaves.find(filename);
    if (it == _fileWaves.end())
        return nullptr;

    return it->second.get();
}

void WavetablePool::clearFileWaves()
{
    _fileWaves.clear();
}

const WavetableMulti* WavetablePool::createFileWave(FilePool& filePool, const std::string& filename)
{
    if (const WavetableMulti* wave = getFileWave(filename))
        return wave;

    if (!filePool.preloadFile(filename, 0))
        return nullptr;

    FilePromisePtr fp = filePool.getFilePromise(filename);
    if (!fp)
        return nullptr;

    fp->waitCompletion();
    if (fp->dataStatus == FilePromise::DataStatus::Error)
        return nullptr;

    // use 1 channel only, maybe warn if file has more channels
    auto audioData = fp->fileData.getSpan(0);
    size_t fftSize = audioData.size();
    size_t specSize = fftSize / 2 + 1;

    typedef std::complex<kiss_fft_scalar> cpx;
    std::unique_ptr<cpx[]> spec { new cpx[specSize] };

    kiss_fftr_cfg cfg = kiss_fftr_alloc(fftSize, false, nullptr, nullptr);
    if (!cfg)
        throw std::bad_alloc();

    kiss_fftr(cfg, audioData.data(), reinterpret_cast<kiss_fft_cpx*>(spec.get()));
    kiss_fftr_free(cfg);

    // scale transform, and normalize amplitude and phase
    const std::complex<double> k = std::polar(2.0 / fftSize, -M_PI / 2);
    for (size_t i = 0; i < specSize; ++i)
        spec[i] *= k;

    TabulatedHarmonicProfile hp {
        absl::Span<const std::complex<float>> { spec.get(), specSize }
    };

    auto wave = std::make_shared<WavetableMulti>(
        WavetableMulti::createForHarmonicProfile(hp, 1.0));

    _fileWaves[filename] = wave;
    return wave.get();
}

} // namespace sfz
