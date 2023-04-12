// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "GUIDefs.h"

#include "utility/vstgui_before.h"
#include "vstgui/lib/ccolor.h"
#include "utility/vstgui_after.h"

namespace gui {

    const CColor kColorTransparent { CColor(0x00, 0x00, 0x00, 0x00) };
    const CColor kColorTransparentDark { CColor(0x00, 0x00, 0x00, 0xc0) };

    const CColor kColorOrange { CColor(0xfd, 0x98, 0x00, 0xff) };

    const CColor kColorControlsScrollerTransparency { CColor(0x00, 0x00, 0x00, 0x80) };
    const CColor kColorControlsTransparency { CColor(0x00, 0x00, 0x00, 0x80) };
    const CColor kColorInfoTransparency { CColor(0x00, 0x00, 0x00, 0x99) };
    const CColor kColorMeterDanger { CColor(0xaa, 0x00, 0x00) };
    const CColor kColorMeterNormal { CColor(0x00, 0xaa, 0x11) };
    const CColor kColorTooltipBackground { CColor(0xff, 0xff, 0xd2, 0xff) };
}
