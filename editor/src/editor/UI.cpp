// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Editor.h"
#include "UI.h"

constexpr el::color UI::bkd_color;

UI::UI(el::view& group)
    : pageHome(std::make_shared<PageHome>(group))
    , pageSettings(std::make_shared<PageSettings>(group))
{
    group.content(
        el::hmin_size(Editor::fixedWidth,
            el::vmin_size(Editor::fixedHeight,
                el::vtile(
                    el::vnotebook(
                        group,
                        el::deck(el::hold(pageHome->contents()),
                            el::hold(pageSettings->contents())),
                        el::tab("Home"), el::tab("Settings"))))),
        background);
}
