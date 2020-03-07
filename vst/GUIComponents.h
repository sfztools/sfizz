// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "vstgui/lib/controls/cslider.h"
#include "vstgui/lib/ccolor.h"

using namespace VSTGUI;

class SimpleSlider : public CSliderBase {
public:
    SimpleSlider(const CRect& bounds, IControlListener* listener, int32_t tag);
    void draw(CDrawContext* dc) override;

    CLASS_METHODS(SimpleSlider, CSliderBase)

private:
    CColor _frame = CColor(0x00, 0x00, 0x00);
    CColor _fill = CColor(0x00, 0x00, 0x00);
};
