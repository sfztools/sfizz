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

    CFontRef getFont() const;
    void setFont(CFontRef font);

    unsigned getNumOctaves() const;
    void setNumOctaves(unsigned octs);

    void setKeyUsed(unsigned key, bool used);
    void setKeyswitchUsed(unsigned key, bool used);
    void setKeyValue(unsigned key, float value);

    enum class KeyRole : int {
        Unused = 0,
        Note   = 1 << 0,
        Switch = 1 << 1,
    };

    KeyRole getKeyRole(unsigned key);

    std::function<void(unsigned, float)> onKeyPressed;
    std::function<void(unsigned, float)> onKeyReleased;

    void setBackColor(const CColor&);
    void setFontColor(const CColor&);

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
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
