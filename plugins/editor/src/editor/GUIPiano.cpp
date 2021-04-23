// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GUIPiano.h"
#include "ColorHelpers.h"
#include "utility/vstgui_before.h"
#include "vstgui/lib/cdrawcontext.h"
#include "vstgui/lib/cgraphicspath.h"
#include "utility/vstgui_after.h"
#include <algorithm>
#include <cmath>

static constexpr CCoord keyoffs[12] = {0,    0.6, 1,   1.8, 2,    3,
                                       3.55, 4,   4.7, 5,   5.85, 6};
static constexpr bool black[12] = {0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0};

struct SPiano::Impl {
    unsigned octs_ {};
    std::unique_ptr<float[]> keyval_ { new float[128]{} };
    std::bitset<128> keyUsed_;
    std::bitset<128> keyswitchUsed_;
    unsigned mousePressedKey_ = ~0u;

    CCoord innerPaddingX_ = 4.0;
    CCoord innerPaddingY_ = 4.0;
    CCoord spacingY_ = 4.0;

    CColor backgroundFill_ { 0xca, 0xca, 0xca, 0xff };
    float backgroundRadius_ = 5.0;

    float keyUsedHue_ = 0.55;
    float keySwitchHue_ = 0.0;
    float whiteKeyChroma_ = 0.9;
    float blackKeyChroma_ = 0.75;
    float whiteKeyLuma_ = 0.9;
    float blackKeyLuma_ = 0.35;
    float keyLumaPressDelta_ = 0.2;

    CColor outline_ { 0x00, 0x00, 0x00, 0xff };
    CColor shadeOutline_ { 0x80, 0x80, 0x80, 0xff };
    CColor labelStroke_ { 0x63, 0x63, 0x63, 0xff };

    mutable Dimensions dim_;
    SharedPointer<CFontDesc> font_;
};

SPiano::SPiano(CRect bounds)
    : CView(bounds), impl_(new Impl)
{
    setNumOctaves(10);
}

CFontRef SPiano::getFont() const
{
    const Impl& impl = *impl_;
    return impl.font_;
}

void SPiano::setFont(CFontRef font)
{
    Impl& impl = *impl_;
    impl.font_ = font;
    getDimensions(true);
    invalid();
}

unsigned SPiano::getNumOctaves() const
{
    const Impl& impl = *impl_;
    return impl.octs_;
}

void SPiano::setNumOctaves(unsigned octs)
{
    Impl& impl = *impl_;
    impl.octs_ = std::max(1u, octs);
    getDimensions(true);
    invalid();
}

void SPiano::setKeyUsed(unsigned key, bool used)
{
    Impl& impl = *impl_;

    if (key >= 128)
        return;

    if (impl.keyUsed_.test(key) == used)
        return;

    impl.keyUsed_.set(key, used);
    invalid();
}

void SPiano::setKeyswitchUsed(unsigned key, bool used)
{
    Impl& impl = *impl_;

    if (key >= 128)
        return;

    if (impl.keyswitchUsed_.test(key) == used)
        return;

    impl.keyswitchUsed_.set(key, used);
    invalid();
}

void SPiano::setKeyValue(unsigned key, float value)
{
    Impl& impl = *impl_;

    if (key >= 128)
        return;

    value = std::max(0.0f, std::min(1.0f, value));

    if (impl.keyval_[key] == value)
        return;

    impl.keyval_[key] = value;
    invalid();
}

SPiano::KeyRole SPiano::getKeyRole(unsigned key)
{
    Impl& impl = *impl_;
    KeyRole role = KeyRole::Unused;

    if (key < 128) {
        if (impl.keyswitchUsed_.test(key))
            role = KeyRole::Switch;
        else if (impl.keyUsed_.test(key))
            role = KeyRole::Note;
    }

    return role;
}

void SPiano::setBackColor(const CColor &color)
{
    Impl& impl = *impl_;
    if (impl.backgroundFill_ != color) {
        impl.backgroundFill_ = color;
        invalid();
    }
}

void SPiano::setFontColor(const CColor &color)
{
    Impl& impl = *impl_;
    if (impl.labelStroke_ != color) {
        impl.labelStroke_ = color;
        invalid();
    }
}

void SPiano::draw(CDrawContext* dc)
{
    Impl& impl = *impl_;
    const Dimensions dim = getDimensions(false);
    const unsigned octs = impl.octs_;
    const unsigned keyCount = octs * 12;
    const bool allKeysUsed = impl.keyUsed_.all();

    dc->setDrawMode(kAntiAliasing);

    if (impl.backgroundFill_.alpha > 0) {
        SharedPointer<CGraphicsPath> path;
        path = owned(dc->createGraphicsPath());
        path->addRoundRect(dim.bounds, impl.backgroundRadius_);
        dc->setFillColor(impl.backgroundFill_);
        dc->drawGraphicsPath(path, CDrawContext::kPathFilled);
    }

    for (unsigned key = 0; key < keyCount; ++key) {
        if (!black[key % 12]) {
            CRect rect = keyRect(key);

            SColorHCY hcy(0.0, impl.whiteKeyChroma_, impl.whiteKeyLuma_);

            switch (getKeyRole(key)) {
            case KeyRole::Note:
                if (allKeysUsed)
                    goto whiteKeyDefault;
                hcy.h = impl.keyUsedHue_;
                break;
            case KeyRole::Switch:
                hcy.h = impl.keySwitchHue_;
                break;
            default: whiteKeyDefault:
                hcy.y = 1.0;
                if (impl.keyval_[key])
                    hcy.c = 0.0;
                break;
            }

            if (impl.keyval_[key])
                hcy.y = std::max(0.0f, hcy.y - impl.keyLumaPressDelta_);

            CColor keycolor = hcy.toColor();
            dc->setFillColor(keycolor);
            dc->drawRect(rect, kDrawFilled);
        }
    }

    dc->setFrameColor(impl.outline_);
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

            SColorHCY hcy(0.0, impl.blackKeyChroma_, impl.blackKeyLuma_);

            switch (getKeyRole(key)) {
            case KeyRole::Note:
                if (allKeysUsed)
                    goto blackKeyDefault;
                hcy.h = impl.keyUsedHue_;
                break;
            case KeyRole::Switch:
                hcy.h = impl.keySwitchHue_;
                break;
            default: blackKeyDefault:
                hcy.c = 0.0;
                break;
            }

            if (impl.keyval_[key])
                hcy.y = std::min(1.0f, hcy.y + impl.keyLumaPressDelta_);

            CColor keycolor = hcy.toColor();
            dc->setFillColor(keycolor);
            dc->drawRect(rect, kDrawFilled);
            dc->setFrameColor(impl.outline_);
            dc->drawRect(rect);
        }
    }

    if (const CFontRef& font = impl.font_) {
        for (unsigned o = 0; o < octs; ++o) {
            CRect rect = keyRect(o * 12);
            CRect textRect(
                rect.left, dim.labelBounds.top,
                rect.right, dim.labelBounds.bottom);
            dc->setFont(font);
            dc->setFontColor(impl.labelStroke_);
            std::string text = std::to_string(static_cast<int>(o) - 1);
            dc->drawString(text.c_str(), textRect, kCenterText);
        }
    }

    {
        dc->setFrameColor(impl.outline_);
        dc->drawLine(dim.keyBounds.getTopLeft(), dim.keyBounds.getTopRight());
        dc->setFrameColor(impl.shadeOutline_);
        dc->drawLine(dim.keyBounds.getBottomLeft(), dim.keyBounds.getBottomRight());
    }

    dc->setFrameColor(impl.outline_);
}

CMouseEventResult SPiano::onMouseDown(CPoint& where, const CButtonState& buttons)
{
    Impl& impl = *impl_;
    unsigned key = keyAtPos(where);
    if (key != ~0u) {
        impl.keyval_[key] = 1;
        impl.mousePressedKey_ = key;
        if (onKeyPressed)
            onKeyPressed(key, mousePressVelocity(key, where.y));
        invalid();
        return kMouseEventHandled;
    }
    return CView::onMouseDown(where, buttons);
}

CMouseEventResult SPiano::onMouseUp(CPoint& where, const CButtonState& buttons)
{
    Impl& impl = *impl_;
    unsigned key = impl.mousePressedKey_;
    if (key != ~0u) {
        impl.keyval_[key] = 0;
        if (onKeyReleased)
            onKeyReleased(key, mousePressVelocity(key, where.y));
        impl.mousePressedKey_ = ~0u;
        invalid();
        return kMouseEventHandled;
    }
    return CView::onMouseUp(where, buttons);
}

CMouseEventResult SPiano::onMouseMoved(CPoint& where, const CButtonState& buttons)
{
    Impl& impl = *impl_;
    if (impl.mousePressedKey_ != ~0u) {
        unsigned key = keyAtPos(where);
        if (impl.mousePressedKey_ != key) {
            impl.keyval_[impl.mousePressedKey_] = 0;
            if (onKeyReleased)
                onKeyReleased(impl.mousePressedKey_, mousePressVelocity(key, where.y));
            // mousePressedKey_ = ~0u;
            if (key != ~0u) {
                impl.keyval_[key] = 1;
                impl.mousePressedKey_ = key;
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
    const Impl& impl = *impl_;

    if (!forceUpdate && impl.dim_.bounds == getViewSize())
        return impl.dim_;

    Dimensions dim;
    dim.bounds = getViewSize();
    dim.paddedBounds = CRect(dim.bounds)
        .extend(-2 * impl.innerPaddingX_, -2 * impl.innerPaddingY_);
    CCoord keyHeight = std::floor(dim.paddedBounds.getHeight());
    CCoord fontHeight = impl.font_ ? impl.font_->getSize() : 0.0;
    keyHeight -= impl.spacingY_ + fontHeight;
    dim.keyBounds = CRect(dim.paddedBounds)
        .setHeight(keyHeight);
    dim.keyWidth = static_cast<unsigned>(
        dim.paddedBounds.getWidth() / impl.octs_ / 7.0);
    dim.keyBounds.setWidth(dim.keyWidth * impl.octs_ * 7.0);
    dim.keyBounds.offset(
        std::floor(0.5 * (dim.paddedBounds.getWidth() - dim.keyBounds.getWidth())), 0.0);

    if (!impl.font_)
        dim.labelBounds = CRect();
    else
        dim.labelBounds = CRect(
            dim.keyBounds.left, dim.keyBounds.bottom + impl.spacingY_,
            dim.keyBounds.right, dim.keyBounds.bottom + impl.spacingY_ + fontHeight);

    impl.dim_ = dim;
    return impl.dim_;
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
    const Impl& impl = *impl_;
    const unsigned octs = impl.octs_;

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
