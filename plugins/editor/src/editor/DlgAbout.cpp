// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "DlgAbout.h"
#include "utility/vstgui_before.h"
#include "vstgui/vstgui.h"
#include "utility/vstgui_after.h"

SAboutDialog::SAboutDialog(const CRect& bounds)
    : CViewContainer(bounds)
{
    setBackgroundColor(CColor(0x00, 0x00, 0x00, 0xc0));

    CView* aboutView = nullptr;
    {
        auto createLogicalGroup = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            CViewContainer* container = new CViewContainer(bounds);
            container->setBackgroundColor(CColor(0x00, 0x00, 0x00, 0x00));
            return container;
        };

        auto createAboutFrame = [](const CRect& bounds, int, const char*, CHoriTxtAlign, int) {
            CViewContainer* container = new CViewContainer(bounds);
            // TODO
            return container;
        };

        auto createLabel = [](const CRect& bounds, int, const char* label, CHoriTxtAlign align, int fontsize) {
            CTextLabel* lbl = new CTextLabel(bounds, label);
            lbl->setFrameColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setBackColor(CColor(0x00, 0x00, 0x00, 0x00));
            lbl->setFontColor(CColor(0xff, 0xff, 0xff, 0xff));
            lbl->setHoriAlign(align);
            auto font = makeOwned<CFontDesc>("Roboto", fontsize);
            lbl->setFont(font);
            return lbl;
        };

        #include "layout/about.hpp"
    }
    addView(aboutView);

    CRect aboutBounds = aboutView->getViewSize();
    aboutBounds.centerInside(CRect(bounds).originize());

    aboutView->setViewSize(aboutBounds);
}

CMouseEventResult SAboutDialog::onMouseDown(CPoint& where, const CButtonState& buttons)
{
    CMouseEventResult result = CViewContainer::onMouseDown(where, buttons);

    if (result != kMouseEventHandled) {
        setVisible(false);
        result = kMouseEventHandled;
    }

    return result;
}
