// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "base/source/fstreamer.h"
#include <string>

using namespace Steinberg;

class SfizzVstState {
public:
    std::string sfzFile;

    static constexpr uint64 currentStateVersion = 0;

    tresult load(IBStream* state);
    tresult store(IBStream* state) const;
};
