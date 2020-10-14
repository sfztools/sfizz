// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GUIPiano.h"
#include "utility/vstgui_before.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "utility/vstgui_after.h"
#include <algorithm>
#include <cmath>

static constexpr CCoord keyoffs[12] = {0,    0.6, 1,   1.8, 2,    3,
                                       3.55, 4,   4.7, 5,   5.85, 6};
static constexpr bool black[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};

SPiano::SPiano(CRect bounds)
    : CView(bounds)
{
    setNumOctaves(10);
}

void SPiano::setFont(CFontRef font)
{
    font_ = font;
    getDimensions(true);
    invalid();
}

void SPiano::setNumOctaves(unsigned octs)
{
    keyval_.resize(octs * 12);
    octs_ = std::max(1u, octs);
    getDimensions(true);
    invalid();
}

void SPiano::draw(CDrawContext* dc)
{
    const Dimensions dim = getDimensions(false);
    const unsigned octs = octs_;
    const unsigned keyCount = octs * 12;

    dc->setDrawMode(kAntiAliasing);

    if (backgroundFill_.alpha > 0) {
        SharedPointer<CGraphicsPath> path;
        path = owned(dc->createGraphicsPath());
        path->addRoundRect(dim.bounds, backgroundRadius_);
        dc->setFillColor(CColor(0xca, 0xca, 0xca));
        dc->drawGraphicsPath(path, CDrawContext::kPathFilled);
    }

    for (unsigned key = 0; key < keyCount; ++key) {
        if (!black[key % 12]) {
            CRect rect = keyRect(key);
            CColor keycolor = whiteFill_;
            if (keyval_[key])
                keycolor = pressedFill_;
            dc->setFillColor(keycolor);
            dc->drawRect(rect, kDrawFilled);
        }
    }

    dc->setFrameColor(outline_);
    dc->drawLine(dim.keyBounds.getTopLeft(), dim.keyBounds.getBottomLeft());
    for (unsigned key = 0; key < keyCount; ++key) {
        if (!black[key % 12]) {
            CRect rect = keyRect(key);
            dc->drawLine(rect.getTopRight(), rect.getBottomRight());
        }
    }

    for (unsigned key = 0; key < keyCount; ++key) {
        if (black[key % 12]) {
            CRect rect = keyRect(key);
            CColor keycolor = blackFill_;
            if (keyval_[key])
                keycolor = pressedFill_;
            dc->setFillColor(keycolor);
            dc->drawRect(rect, kDrawFilled);
            dc->setFrameColor(outline_);
            dc->drawRect(rect);
        }
    }

    if (const CFontRef& font = font_) {
        for (unsigned o = 0; o < octs; ++o) {
            CRect rect = keyRect(o * 12);
            CRect textRect(
                rect.left, dim.labelBounds.top,
                rect.right, dim.labelBounds.bottom);
            dc->setFont(font_);
            dc->setFontColor(labelStroke_);
            std::string text = std::to_string(static_cast<int>(o) - 1);
            dc->drawString(text.c_str(), textRect, kCenterText);
        }
    }

    {
        dc->setFrameColor(outline_);
        dc->drawLine(dim.keyBounds.getTopLeft(), dim.keyBounds.getTopRight());
        dc->setFrameColor(shadeOutline_);
        dc->drawLine(dim.keyBounds.getBottomLeft(), dim.keyBounds.getBottomRight());
    }

    dc->setFrameColor(outline_);
}

CMouseEventResult SPiano::onMouseDown(CPoint& where, const CButtonState& buttons)
{
    unsigned key = keyAtPos(where);
    if (key != ~0u) {
        keyval_[key] = 1;
        mousePressedKey_ = key;
        if (onKeyPressed)
            onKeyPressed(key, mousePressVelocity(key, where.y));
        invalid();
        return kMouseEventHandled;
    }
    return CView::onMouseDown(where, buttons);
}

CMouseEventResult SPiano::onMouseUp(CPoint& where, const CButtonState& buttons)
{
    unsigned key = mousePressedKey_;
    if (key != ~0u) {
        keyval_[key] = 0;
        if (onKeyReleased)
            onKeyReleased(key, mousePressVelocity(key, where.y));
        mousePressedKey_ = ~0u;
        invalid();
        return kMouseEventHandled;
    }
    return CView::onMouseUp(where, buttons);
}

CMouseEventResult SPiano::onMouseMoved(CPoint& where, const CButtonState& buttons)
{
    if (mousePressedKey_ != ~0u) {
        unsigned key = keyAtPos(where);
        if (mousePressedKey_ != key) {
            keyval_[mousePressedKey_] = 0;
            if (onKeyReleased)
                onKeyReleased(mousePressedKey_, mousePressVelocity(key, where.y));
            // mousePressedKey_ = ~0u;
            if (key != ~0u) {
                keyval_[key] = 1;
                mousePressedKey_ = key;
                if (onKeyPressed)
                    onKeyPressed(key, mousePressVelocity(key, where.y));
            }
            invalid();
        }
        return kMouseEventHandled;
    }
    return CView::onMouseMoved(where, buttons);
}

const SPiano::Dimensions& SPiano::getDimensions(bool forceUpdate) const
{
    if (!forceUpdate && dim_.bounds == getViewSize())
        return dim_;

    Dimensions dim;
    dim.bounds = getViewSize();
    dim.paddedBounds = CRect(dim.bounds)
        .extend(-2 * innerPaddingX_, -2 * innerPaddingY_);
    CCoord keyHeight = std::floor(dim.paddedBounds.getHeight());
    CCoord fontHeight = font_ ? font_->getSize() : 0.0;
    keyHeight -= spacingY_ + fontHeight;
    dim.keyBounds = CRect(dim.paddedBounds)
        .setHeight(keyHeight);
    dim.keyWidth = static_cast<unsigned>(
        dim.paddedBounds.getWidth() / octs_ / 7.0);
    dim.keyBounds.setWidth(dim.keyWidth * octs_ * 7.0);
    dim.keyBounds.offset(
        std::floor(0.5 * (dim.paddedBounds.getWidth() - dim.keyBounds.getWidth())), 0.0);

    if (!font_)
        dim.labelBounds = CRect();
    else
        dim.labelBounds = CRect(
            dim.keyBounds.left, dim.keyBounds.bottom + spacingY_,
            dim.keyBounds.right, dim.keyBounds.bottom + spacingY_ + fontHeight);

    dim_ = dim;
    return dim_;
}

CRect SPiano::keyRect(const Dimensions& dim, unsigned key)
{
    unsigned oct = key / 12;
    unsigned note = key % 12;
    unsigned keyw = dim.keyWidth;
    unsigned keyh = static_cast<unsigned>(dim.keyBounds.getHeight());
    CCoord octwidth = (keyoffs[11] + 1.0) * keyw;
    CCoord octx = octwidth * oct;
    CCoord notex = octx + keyoffs[note] * keyw;
    CCoord notew = black[note] ? (0.6 * keyw) : keyw;
    CCoord noteh = black[note] ? (0.6 * keyh) : keyh;
    return CRect(notex, 0.0, notex + notew, noteh).offset(dim.keyBounds.getTopLeft());
}

CRect SPiano::keyRect(unsigned key) const
{
    return keyRect(getDimensions(false), key);
}

unsigned SPiano::keyAtPos(CPoint pos) const
{
    const unsigned octs = octs_;

    for (unsigned key = 0; key < octs * 12; ++key) {
        if (black[key % 12]) {
            if (keyRect(key).pointInside(pos))
                return key;
        }
    }

    for (unsigned key = 0; key < octs * 12; ++key) {
        if (!black[key % 12]) {
            if (keyRect(key).pointInside(pos))
                return key;
        }
    }

    return ~0u;
}

float SPiano::mousePressVelocity(unsigned key, CCoord posY)
{
    const CRect rect = keyRect(key);
    CCoord value = (posY - rect.top) / rect.getHeight();
    return std::max(0.0f, std::min(1.0f, static_cast<float>(value)));
}
