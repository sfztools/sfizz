// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "utility/vstgui_before.h"
#include "vstgui/lib/cviewcontainer.h"
#include "utility/vstgui_after.h"

using namespace VSTGUI;

class SAboutDialog : public CViewContainer {
public:
    explicit SAboutDialog(const CRect& bounds);

protected:
    CMouseEventResult onMouseDown(CPoint& where, const CButtonState& buttons);
};
