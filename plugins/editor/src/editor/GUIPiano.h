// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "utility/vstgui_before.h"
#include "vstgui/lib/cview.h"
#include "vstgui/lib/ccolor.h"
#include "utility/vstgui_after.h"
#include <functional>
#include <vector>
#include <bitset>

using namespace VSTGUI;

class SPiano : public CView {
public:
    explicit SPiano(CRect bounds);

    CFontRef getFont() const { return font_; }
    void setFont(CFontRef font);

    unsigned getNumOctaves() const { return octs_; }
    void setNumOctaves(unsigned octs);

    void setKeyUsed(unsigned key, bool used);

    std::function<void(unsigned, float)> onKeyPressed;
    std::function<void(unsigned, float)> onKeyReleased;

protected:
    void draw(CDrawContext* dc) override;
    CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons) override;
    CMouseEventResult onMouseUp(CPoint& where, const CButtonState& buttons) override;
    CMouseEventResult onMouseMoved(CPoint& where, const CButtonState& buttons) override;

private:
    struct Dimensions {
        CRect bounds {};
        CRect paddedBounds {};
        CRect keyBounds {};
        unsigned keyWidth {};
        CRect labelBounds {};
    };
    const Dimensions& getDimensions(bool forceUpdate) const;

    static CRect keyRect(const Dimensions& dim, unsigned key);
    CRect keyRect(unsigned key) const;
    unsigned keyAtPos(CPoint pos) const;

    float mousePressVelocity(unsigned key, CCoord posY);

private:
    unsigned octs_ {};
    std::vector<unsigned> keyval_;
    std::bitset<128> keyUsed_;
    unsigned mousePressedKey_ = ~0u;

    CCoord innerPaddingX_ = 4.0;
    CCoord innerPaddingY_ = 4.0;
    CCoord spacingY_ = 4.0;

    CColor backgroundFill_ { 0xca, 0xca, 0xca, 0xff };
    float backgroundRadius_ = 5.0;

    float keyUsedHue_ = 0.55;
    float whiteKeyLuma_ = 0.5;
    float blackKeyLuma_ = 0.25;
    float keyLumaPressDelta_ = 0.20;

    CColor outline_ { 0x00, 0x00, 0x00, 0xff };
    CColor shadeOutline_ { 0x80, 0x80, 0x80, 0xff };
    CColor labelStroke_ { 0x63, 0x63, 0x63, 0xff };

    mutable Dimensions dim_;
    SharedPointer<CFontDesc> font_;
};
