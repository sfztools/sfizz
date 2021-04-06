// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "utility/vstgui_before.h"
#include "vstgui/vstgui.h"
#include "utility/vstgui_after.h"

#include <string>

using namespace VSTGUI;

struct Palette {
    CColor boxBackground;
    CColor text;
    CColor inactiveText;
    CColor highlightedText;
    CColor titleBoxText;
    CColor titleBoxBackground;
    CColor icon;
    CColor iconHighlight;
    CColor valueText;
    CColor valueBackground;
    CColor knobActiveTrack;
    CColor knobInactiveTrack;
    CColor knobLineIndicator;
};

struct Theme {
    Theme();
    void clear();
    void load(const std::string& name);

    static void storeCurrentName(const std::string& name);
    static std::string getCurrentName();

    int getCurrentIndex() const;

    CColor* getColorFromName(const std::string& name, bool fromInvertedPalette = false);
    std::vector<UTF8String> getNames() const;

    CColor frameBackground;
    Palette normalPalette;
    Palette invertedPalette;
};
