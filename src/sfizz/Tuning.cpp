// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Tuning.h"
#include "Debug.h"
#include "absl/types/optional.h"
#include "Tunings.h" // Surge tuning library
#include <fstream>
#include <sstream>
#include <array>
#include <cmath>

namespace sfz {

struct Tuning::Impl {
public:
    Impl() { updateKeysFractional12TET(); }

    const Tunings::Tuning& tuning() const { return tuning_; }

    float getKeyFractional12TET(int midiKey) const;

    int rootKey() const { return rootKey_; }
    float tuningFrequency() const { return tuningFrequency_; }

    void updateScale(const Tunings::Scale& scale, absl::optional<fs::path> sourceFile = {});
    bool shouldReloadScala();
    void updateRootKey(int rootKey);
    void updateTuningFrequency(float tuningFrequency);
    void reset();
private:
    void updateKeysFractional12TET();
    static Tunings::KeyboardMapping mappingFromParameters(int rootKey, float tuningFrequency);

private:
    static constexpr int defaultRootKey = 60;
    static constexpr float defaultTuningFrequency = 440.0;
    int rootKey_ = defaultRootKey;
    float tuningFrequency_ = defaultTuningFrequency;

    Tunings::Tuning tuning_ {
        Tunings::evenTemperament12NoteScale(),
        mappingFromParameters(defaultRootKey, defaultTuningFrequency)
    };

    absl::optional<fs::path> scalaFile_;
    fs::file_time_type modificationTime_ {};

    static constexpr int numKeys = Tunings::Tuning::N;
    static constexpr int keyOffset = 256; // Surge tuning has key range ±256
    std::array<float, numKeys> keysFractional12TET_;
};

void Tuning::Impl::reset()
{
    rootKey_ = defaultRootKey;
    tuningFrequency_ = defaultTuningFrequency;
    tuning_ = Tunings::Tuning(
        Tunings::evenTemperament12NoteScale(),
        mappingFromParameters(defaultRootKey, defaultTuningFrequency)
    );
    scalaFile_.reset();
    modificationTime_ = fs::file_time_type::min();
    updateKeysFractional12TET();
}

float Tuning::Impl::getKeyFractional12TET(int midiKey) const
{
    return keysFractional12TET_[std::max(0, std::min(numKeys - 1, midiKey + keyOffset))];
}

void Tuning::Impl::updateScale(const Tunings::Scale& scale, absl::optional<fs::path> sourceFile)
{
    tuning_ = Tunings::Tuning(scale, tuning_.keyboardMapping);
    updateKeysFractional12TET();

    if (sourceFile) {
        std::error_code ec;
        modificationTime_ = fs::last_write_time(*sourceFile, ec);
        scalaFile_ = sourceFile;
    }
}

bool Tuning::Impl::shouldReloadScala()
{
    DBG("Should reload scala called");
    if (!scalaFile_)
        return false;

    std::error_code ec;
    const auto newTime = fs::last_write_time(*scalaFile_, ec);
    if (newTime > modificationTime_) {
        DBG("File changed!");
        modificationTime_ = newTime;
        return true;
    }

    return false;
}

void Tuning::Impl::updateRootKey(int rootKey)
{
    ASSERT(rootKey >= 0);
    rootKey = std::max(0, rootKey);

    if (rootKey_ == rootKey)
        return;

    tuning_ = Tunings::Tuning(tuning_.scale, mappingFromParameters(rootKey, tuningFrequency_));
    rootKey_ = rootKey;
    updateKeysFractional12TET();
}

void Tuning::Impl::updateTuningFrequency(float tuningFrequency)
{
    ASSERT(tuningFrequency >= 0);
    tuningFrequency = std::max(0.0f, tuningFrequency);

    if (tuningFrequency_ == tuningFrequency)
        return;

    tuning_ = Tunings::Tuning(tuning_.scale, mappingFromParameters(rootKey_, tuningFrequency));
    tuningFrequency_ = tuningFrequency;
    updateKeysFractional12TET();
}

void Tuning::Impl::updateKeysFractional12TET()
{
    // mapping of MIDI key to equal temperament key
    for (int key = 0; key < numKeys; ++key) {
        double freq = tuning_.frequencyForMidiNote(key - keyOffset);
        keysFractional12TET_[key] = 12.0 * std::log2(freq / 440.0) + 69.0;
    }
}

Tunings::KeyboardMapping Tuning::Impl::mappingFromParameters(int rootKey, float tuningFrequency)
{
#if 1
    // root note is the start of octave. like Scala
#else
    // root note is the start of next octave. like Sforzando
    rootKey = std::max(0, rootKey - 12);
#endif
    // fixed frequency of the root note
    const double rootFrequency = tuningFrequency * std::exp2((rootKey - 69) / 12.0);
    return Tunings::tuneNoteTo(rootKey, rootFrequency);
}

///
Tuning::Tuning()
    : impl_(new Impl)
{
}

Tuning::~Tuning()
{
}

bool Tuning::loadScalaFile(const fs::path& path)
{
    const Tunings::KeyboardMapping kbm = impl_->tuning().keyboardMapping;

    fs::ifstream stream(path);
    if (stream.bad()) {
        DBG("Cannot open scale file: " << path);
        return false;
    }

    Tunings::Scale scl;
    try {
        scl = Tunings::readSCLStream(stream);
    }
    catch (Tunings::TuningError& error) {
        DBG("Tuning: " << error.what());
        return false;
    }

    impl_->updateScale(scl, path);
    return true;
}

bool Tuning::loadScalaString(const std::string& text)
{
    const Tunings::KeyboardMapping kbm = impl_->tuning().keyboardMapping;

    std::istringstream stream(text);

    Tunings::Scale scl;
    try {
        scl = Tunings::readSCLStream(stream);
    }
    catch (Tunings::TuningError& error) {
        DBG("Tuning: " << error.what());
        return false;
    }

    impl_->updateScale(scl);
    return true;
}

void Tuning::setScalaRootKey(int rootKey)
{
    impl_->updateRootKey(rootKey);
}

int Tuning::getScalaRootKey() const
{
    return impl_->rootKey();
}

void Tuning::setTuningFrequency(float frequency)
{
    impl_->updateTuningFrequency(frequency);
}

float Tuning::getTuningFrequency() const
{
    return impl_->tuningFrequency();
}

void Tuning::loadEqualTemperamentScale()
{
    impl_->updateScale(Tunings::evenTemperament12NoteScale());
}

float Tuning::getFrequencyOfKey(int midiKey) const
{
    return impl_->tuning().frequencyForMidiNote(midiKey);
}

float Tuning::getKeyFractional12TET(int midiKey)
{
    return impl_->getKeyFractional12TET(midiKey);
}

bool Tuning::shouldReloadScala()
{
    return impl_->shouldReloadScala();
}


} // namespace sfz
