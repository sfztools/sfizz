// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Editor.h"
#include "EditorController.h"
#include "UI.h"
#include "PageHome.h"
#include "PageSettings.h"

constexpr el::color UI::bkd_color;

UI::UI(el::view& group, EditorController& ctrl)
    : ctrl_(&ctrl)
    , pageHome(std::make_shared<PageHome>(group))
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

    pageHome->on_change_volume = [this](double v) {
        ctrl_->uiSendNumber(EditId::Volume, v);
    };
    // TODO the other numeric values...

    pageHome->on_change_sfz_file = [this](absl::string_view v) {
        ctrl_->uiSendString(EditId::Volume, v);
    };
    // TODO the other string values...
}

UI::~UI()
{
}

void UI::updateVolume(float v)
{
    pageHome->updateVolume(v);
}

void UI::updateSfzFile(cycfi::string_view v)
{
    pageHome->updateSfzFile(v);
}
