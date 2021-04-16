// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "sfizz/Config.h"
#include "sfizz/utility/bit_array/BitArray.h"
#include <absl/strings/string_view.h>
#include <string>
#include <array>
#include <iosfwd>
struct sfizz_synth_t;

/**
 * @brief Description of user-interactible elements of the SFZ instrument
 */
struct InstrumentDescription {
    uint32_t numRegions {};
    uint32_t numGroups {};
    uint32_t numMasters {};
    uint32_t numCurves {};
    uint32_t numSamples {};
    std::string rootPath;
    std::string image;
    BitArray<128> keyUsed {};
    BitArray<128> keyswitchUsed {};
    BitArray<sfz::config::numCCs> ccUsed {};
    std::array<std::string, 128> keyLabel {};
    std::array<std::string, 128> keyswitchLabel {};
    std::array<std::string, sfz::config::numCCs> ccLabel {};
    std::array<float, sfz::config::numCCs> ccDefault {};
};

/**
 * @brief Produce a description of the currently loaded instrument in the synth,
 *        in the form of a concatenation of OSC messages.
 *
 * This form is a message transmissible over binary channels.
 */
std::string getDescriptionBlob(sfizz_synth_t* handle);

/**
 * @brief Extract the information from the OSC blob and rearrange it in a
 *        structured form.
 */
InstrumentDescription parseDescriptionBlob(absl::string_view blob);

/**
 * @brief Display the description in human-readable format.
 */
std::ostream& operator<<(std::ostream& os, const InstrumentDescription& desc);
