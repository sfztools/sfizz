// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "utility/vstgui_before.h"
#include "vstgui/vstgui.h"
#include "utility/vstgui_after.h"
#include <absl/strings/string_view.h>
#include <string>
#include <vector>
namespace pugi { class xml_document; }

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
    CColor knobText;
    CColor knobLabelText;
    CColor knobLabelBackground;
};

struct Theme {
    Theme() = default;

    Theme(const Theme&) = delete;
    Theme& operator=(const Theme&) = delete;

    CColor frameBackground;
    Palette normalPalette {};
    Palette invertedPalette {};

    struct ChangeListener {
        virtual ~ChangeListener() {}
        virtual void onThemeChanged() = 0;
    };

    ChangeListener* listener = nullptr;

    void clear();
    void load(const std::string& name);
    void loadDocument(const pugi::xml_document& doc);

    void invokeChangeListener() { if (listener) listener->onThemeChanged(); }

    static void storeCurrentName(absl::string_view name);
    static std::string loadCurrentName();
    static const std::vector<std::string>& getAvailableNames();

    CColor* getColorFromName(absl::string_view name, bool fromInvertedPalette = false);

private:
    static std::vector<std::string> extractAvailableNames();
};
