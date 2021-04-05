// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "sfizz/Synth.h"
#include "sfizz/Region.h"
#include "sfizz/Voice.h"
#include "sfizz/Range.h"
#include "sfizz/Messaging.h"
#include "catch2/catch.hpp"
#include "sfizz/modulations/ModKey.h"

class RegionCCView {
public:
    RegionCCView(const sfz::Region& region, sfz::ModKey target)
        : region_(region), target_(target)
    {
    }

    size_t size() const;
    bool empty() const;
    sfz::ModKey::Parameters at(int cc) const;
    float valueAt(int cc) const;

private:
    bool match(const sfz::Region::Connection& conn) const;

private:
    const sfz::Region& region_;
    sfz::ModKey target_;
};

template<class Range>
void almostEqualRanges(const Range& lhs, const Range& rhs)
{
    REQUIRE(lhs.getStart() == Approx(rhs.getStart()));
    REQUIRE(lhs.getEnd() == Approx(rhs.getEnd()));
}

template<class C>
void sortAll(C& container)
{
    std::sort(container.begin(), container.end());
}

template<class C, class... Args>
void sortAll(C& container, Args&... others)
{
    std::sort(container.begin(), container.end());
    sortAll(others...);
}

/**
 * @brief Get active voices from the synth
 *
 * @param synth
 * @return const std::vector<const sfz::Voice*>
 */
const std::vector<const sfz::Voice*> getActiveVoices(const sfz::Synth& synth);

/**
 * @brief Get playing (unreleased) voices from the synth
 *
 * @param synth
 * @return const std::vector<const sfz::Voice*>
 */
const std::vector<const sfz::Voice*> getPlayingVoices(const sfz::Synth& synth);


/**
 * @brief Count the number of playing (unreleased) voices from the synth
 *
 * @param synth
 * @return unsigned
 */
unsigned numPlayingVoices(const sfz::Synth& synth);

/**
 * @brief Get the playing samples
 *
 * @param synth
 * @return unsigned
 */
const std::vector<std::string> playingSamples(const sfz::Synth& synth);

/**
 * @brief Get the playing notes velocities
 *
 * @param synth
 * @return unsigned
 */
const std::vector<float> playingVelocities(const sfz::Synth& synth);

/**
 * @brief Get the active samples
 *
 * @param synth
 * @return unsigned
 */
const std::vector<std::string> activeSamples(const sfz::Synth& synth);

/**
 * @brief Get the active notes velocities
 *
 * @param synth
 * @return unsigned
 */
const std::vector<float> activeVelocities(const sfz::Synth& synth);

/**
 * @brief Create the default dot graph representation for standard regions
 *
 */
std::string createDefaultGraph(std::vector<std::string> lines, int numRegions = 1);

/**
 * @brief Create a dot graph with the specified lines.
 * The lines are sorted.
 *
 */
std::string createModulationDotGraph(std::vector<std::string> lines);

template <class Type>
inline bool approxEqual(absl::Span<const Type> lhs, absl::Span<const Type> rhs, Type eps = 1e-3)
{
    if (lhs.size() != rhs.size())
        return false;

    for (size_t i = 0; i < rhs.size(); ++i)
        if (rhs[i] != Approx(lhs[i]).epsilon(eps)) {
            std::cerr << lhs[i] << " != " << rhs[i] << " at index " << i << '\n';
            return false;
        }

    return true;
}

/**
 * @brief Simple helper function that feeds all received messages into a std::vector<std::string>* in data.
 */
void simpleMessageReceiver(void* data, int delay, const char* path, const char* sig, const sfizz_arg_t* args);
