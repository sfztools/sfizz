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
#include <iostream>

#define COLOR_CASE(X) \
    case hash(#X): \
        c = &X; \
        break

#define PALETTE_COLOR_CASE(X) \
    case hash(#X): \
        c = fromInvertedPalette ? &invertedPalette.X : &normalPalette.X; \
        break

Theme::Theme()
{
    clear();
}

void Theme::clear() {
    frameBackground = {};
    normalPalette = {};
    invertedPalette = {};
}

enum class PaletteType {
    Normal,
    Inverted,
    Unknown,
    PaletteTypeMax
};

enum class NodeType {
    Color,
    Palette,
    Unknown,
    NodeTypeMax
};

static NodeType getNodeType(const pugi::xml_node& node)
{
    NodeType nodeType = NodeType::Unknown;
    switch (hash(node.name())) {
    case hash("color"):
        nodeType = NodeType::Color;
        break;
    case hash("palette"):
        nodeType = NodeType::Palette;
        break;
    default:
        break;
    }
    return nodeType;
}

static PaletteType paletteTypeFromNode(const pugi::xml_node& node)
{
    PaletteType paletteType = PaletteType::Unknown;
    const char* nodeName = node.attribute("name").as_string();
    if (std::strcmp(nodeName, "normal") == 0 )
        paletteType = PaletteType::Normal;
    else if (std::strcmp(nodeName, "inverted") == 0)
        paletteType = PaletteType::Inverted;

    return paletteType;
}

static void parseColorNode(Theme& theme, const pugi::xml_node& colorNode, PaletteType paletteType)
{
    bool inverted = (paletteType == PaletteType::Inverted) ? true : false;
    std::string colorName = colorNode.attribute("name").as_string();
    CColor* themeColor = theme.getColorFromName(colorName, inverted);
    if (themeColor) {
        *themeColor = {};
        std::string strColor = colorNode.text().as_string();
        if (!colorFromHex(strColor, *themeColor))
            std::cerr << "[sfizz] invalid color " << strColor << '\n';
    } else {
         std::cerr << "[sfizz] invalid color " << colorName << '\n';
    }
}

struct themeWalker : pugi::xml_tree_walker
{
    explicit themeWalker(Theme& t)
        : theme(t)
        , currentPaletteType(PaletteType::Unknown)
    {};

    virtual bool for_each(pugi::xml_node& node) {
        PaletteType paletteType = paletteTypeFromNode(node);
        if (paletteType != PaletteType::Unknown)
            currentPaletteType = paletteType;

        NodeType nodeType = getNodeType(node);
        if (nodeType == NodeType::Color)
            parseColorNode(theme, node, currentPaletteType);

        return true;
    }
    Theme& theme;
    PaletteType currentPaletteType;
};

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
    pugi::xml_node rootNode(doc.child("sfizz-theme"));
    if (!rootNode) {
        std::cerr << "[sfizz] trying to load an invalid theme\n";
        return;
    }
    themeWalker walker(*this);
    doc.traverse(walker);
}

void Theme::storeCurrentName(const std::string& name)
{
    SfizzSettings settings;
    settings.store("current_theme", name);
}

std::string Theme::getCurrentName()
{
    SfizzSettings settings;
    return settings.load_or("current_theme", "Default");
}

int Theme::getCurrentIndex() const
{
    int currentIndex = -1;
    std::vector<UTF8String> names = getNames();
    std::string currentName = getCurrentName();

    auto it = std::find(names.begin(), names.end(), currentName);
    if (it != names.end())
        return std::distance(names.begin(), it);

    return currentIndex;
}

CColor* Theme::getColorFromName(const std::string& name, bool fromInvertedPalette)
{
    CColor* c = nullptr;
    switch (hash(name)) {
        COLOR_CASE(frameBackground);
        PALETTE_COLOR_CASE(boxBackground);
        PALETTE_COLOR_CASE(highlightedText);
        PALETTE_COLOR_CASE(icon);
        PALETTE_COLOR_CASE(iconHighlight);
        PALETTE_COLOR_CASE(inactiveText);
        PALETTE_COLOR_CASE(knobActiveTrack);
        PALETTE_COLOR_CASE(knobInactiveTrack);
        PALETTE_COLOR_CASE(knobLineIndicator);
        PALETTE_COLOR_CASE(text);
        PALETTE_COLOR_CASE(titleBoxBackground);
        PALETTE_COLOR_CASE(titleBoxText);
        PALETTE_COLOR_CASE(valueBackground);
        PALETTE_COLOR_CASE(valueText);
        default: break;
    }
    return c;
}

std::vector<UTF8String> Theme::getNames() const
{
    std::vector<UTF8String> names {};
    std::error_code ec;
    fs::path themesPath = getResourceBasePath() / "Themes";
    fs::directory_iterator it(themesPath, ec);
    if (!ec) {
        for (const auto& entry: it) {
            if (entry.is_directory()) {
                UTF8String themeName(entry.path().filename().u8string());
                names.emplace_back(std::move(themeName));
            }
        }
    } else {
        std::cerr << "[sfizz] error while reading themes path: " << ec.message() << '\n';
    }
    return names;
}
