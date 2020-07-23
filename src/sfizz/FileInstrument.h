// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "ghc/fs_std.hpp"
#include <sndfile.h>

namespace sfz {

class FileInstruments {
public:
/**
 * @brief Extract the loop information of a FLAC file, using RIFF foreign data.
 *
 * This feature lacks support in libsndfile (as of version 1.0.28).
 * see https://github.com/erikd/libsndfile/issues/59
 */
static bool extractFromFlac(const fs::path& path, SF_INSTRUMENT& ins);
};

} // namespace sfz
