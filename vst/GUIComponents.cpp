// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GUIComponents.h"
#include "vstgui/lib/cdrawcontext.h"

SimpleSlider::SimpleSlider(const CRect& bounds, IControlListener* listener, int32_t tag)
    : CSliderBase(bounds, listener, tag)
{
    setStyle(kHorizontal|kLeft);

    CPoint offsetHandle(2.0, 2.0);
    setOffsetHandle(offsetHandle);

    CCoord handleSize = 20.0;
    setHandleSizePrivate(handleSize, bounds.bottom - bounds.top - 2 * offsetHandle.y);
    setHandleRangePrivate(bounds.right - bounds.left - handleSize - 2 * offsetHandle.x);
}

void SimpleSlider::draw(CDrawContext* dc)
{
    CRect bounds = getViewSize();
    CRect handle = calculateHandleRect(getValueNormalized());

    dc->setFrameColor(_frame);
    dc->drawRect(bounds, kDrawStroked);

    dc->setFillColor(_fill);
    dc->drawRect(handle, kDrawFilled);
}
