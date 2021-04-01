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

struct InstrumentDescription {
    BitArray<128> keyUsed {};
    BitArray<128> keyswitchUsed {};
    BitArray<sfz::config::numCCs> ccUsed {};
    std::array<std::string, 128> keyLabel {};
    std::array<std::string, 128> keyswitchLabel {};
    std::array<std::string, sfz::config::numCCs> ccLabel {};
    std::array<float, sfz::config::numCCs> ccDefault {};
};

std::string getDescriptionBlob(sfizz_synth_t* handle);
InstrumentDescription parseDescriptionBlob(absl::string_view blob);

std::ostream& operator<<(std::ostream& os, const InstrumentDescription& desc);
