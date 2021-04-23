// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Theme.h"
#include "ColorHelpers.h"
#include "VSTGUIHelpers.h"
#include "plugin/SfizzSettings.h"
#include "sfizz/utility/StringViewHelpers.h"
#include <pugixml.hpp>
#include <ghc/fs_std.hpp>
#include <algorithm>
#include <iostream>

void Theme::clear() {
    frameBackground = {};
    normalPalette = {};
    invertedPalette = {};
}

void Theme::load(const std::string& name)
{
    fs::path resPath = getResourceBasePath();
    fs::path themePath = resPath / "Themes" / fs::u8path(name) / "theme.xml";

    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(themePath.c_str());
    if (!result) {
        std::cerr << "[sfizz] cannot load theme from " << resPath << '\n';
        return;
    }

    loadDocument(doc);
}

void Theme::loadDocument(const pugi::xml_document& doc)
{
    pugi::xml_node rootNode(doc.child("sfizz-theme"));
    if (!rootNode) {
        std::cerr << "[sfizz] trying to load an invalid theme\n";
        return;
    }

    ///
    clear();

    ///
    auto loadChildColorNodes = [this](pugi::xml_node topNode, bool inverted) {
        for (pugi::xml_node colorNode : topNode.children("color")) {
            absl::string_view name = colorNode.attribute("name").as_string();
            CColor* slot = getColorFromName(name, inverted);
            if (!slot) {
                std::cerr << "[sfizz] color not recognized: " << name << "\n";
                continue;
            }

            *slot = {};

            absl::string_view colorText = colorNode.text().as_string();
            if (!colorFromHex(colorText, *slot))
                std::cerr << "[sfizz] invalid color value: " << colorText << "\n";
        }
    };

    ///
    loadChildColorNodes(rootNode, false);

    for (pugi::xml_node paletteNode : rootNode.children("palette")) {
        absl::string_view paletteName = paletteNode.attribute("name").as_string();

        bool inverted;
        if (paletteName == "normal")
            inverted = false;
        else if (paletteName == "inverted")
            inverted = true;
        else {
            std::cerr << "[sfizz] palette not recognized: " << paletteName << "\n";
            continue;
        }

        loadChildColorNodes(paletteNode, inverted);
    }

    ///
    invokeChangeListener();
}

void Theme::storeCurrentName(absl::string_view name)
{
    SfizzSettings settings;
    settings.store("current_theme", name);
}

std::string Theme::loadCurrentName()
{
    SfizzSettings settings;
    return settings.load_or("current_theme", "Default");
}

const std::vector<std::string>& Theme::getAvailableNames()
{
    static const std::vector<std::string> names = extractAvailableNames();
    return names;
}

std::vector<std::string> Theme::extractAvailableNames()
{
    fs::path themesPath = getResourceBasePath() / "Themes";

    std::error_code ec;
    fs::directory_iterator it(themesPath, ec);
    if (ec) {
        std::cerr << "[sfizz] error reading the theme directory: " << ec.message() << '\n';
        return {};
    }

    std::vector<std::string> names;
    for (; !ec && it != fs::directory_iterator(); it.increment(ec)) {
        const fs::directory_entry& entry = *it;
        if (entry.is_directory())
            names.emplace_back(entry.path().filename().u8string());
    }

    std::sort(
        names.begin(), names.end(),
        [](const std::string& a, const std::string& b) -> bool {
            return (a == "Default") ? true : (b == "Default") ? false : a < b;
        });

    return names;
}

CColor* Theme::getColorFromName(absl::string_view name, bool fromInvertedPalette)
{
    CColor* c = nullptr;
    switch (hash(name)) {
        #define COLOR_CASE(X)                           \
            case hash(#X):                              \
                c = &X;                                 \
                break
        #define PALETTE_COLOR_CASE(X)                           \
            case hash(#X):                                      \
                c = fromInvertedPalette ?                       \
                    &invertedPalette.X : &normalPalette.X;      \
                break

        COLOR_CASE(frameBackground);
        PALETTE_COLOR_CASE(boxBackground);
        PALETTE_COLOR_CASE(highlightedText);
        PALETTE_COLOR_CASE(icon);
        PALETTE_COLOR_CASE(iconHighlight);
        PALETTE_COLOR_CASE(inactiveText);
        PALETTE_COLOR_CASE(knobActiveTrack);
        PALETTE_COLOR_CASE(knobInactiveTrack);
        PALETTE_COLOR_CASE(knobLabelText);
        PALETTE_COLOR_CASE(knobLineIndicator);
        PALETTE_COLOR_CASE(knobText);
        PALETTE_COLOR_CASE(knobLabelBackground);
        PALETTE_COLOR_CASE(text);
        PALETTE_COLOR_CASE(titleBoxBackground);
        PALETTE_COLOR_CASE(titleBoxText);
        PALETTE_COLOR_CASE(valueBackground);
        PALETTE_COLOR_CASE(valueText);

        #undef COLOR_CASE
        #undef PALETTE_COLOR_CASE

        default: break;
    }
    return c;
}
