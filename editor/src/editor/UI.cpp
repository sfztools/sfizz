// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Editor.h"
#include "EditorController.h"
#include "UI.h"
#include "PageHome.h"
#include "PageSettings.h"

// Main window background color
static constexpr el::color bg_color = el::rgba(35, 35, 37, 255);

struct UI::Impl {
    EditorController* ctrl_ = nullptr;
    std::shared_ptr<PageHome> pageHome_;
    std::shared_ptr<PageSettings> pageSettings_;
};

UI::UI(el::view& group, EditorController& ctrl)
    : impl_(new Impl)
{
    impl_->ctrl_ = &ctrl;

    auto pageHome = std::make_shared<PageHome>(group);
    auto pageSettings = std::make_shared<PageSettings>(group);
    impl_->pageHome_ = pageHome;
    impl_->pageSettings_ = pageSettings;

    group.content(
        el::hmin_size(Editor::fixedWidth,
            el::vmin_size(Editor::fixedHeight,
                el::vtile(
                    el::vnotebook(
                        group,
                        el::deck(
                            el::layer(el::hold(pageHome), el::frame{}),
                            el::layer(el::hold(pageSettings), el::frame{})),
                        el::tab("Home"), el::tab("Settings"))))),
        el::box(bg_color));

    std::function<void(EditId, float)> sendNumberCb =
        [this](EditId id, float v) { sendNumber(id, v); };
    std::function<void(EditId, cycfi::string_view)> sendStringCb =
        [this](EditId id, cycfi::string_view v) { sendString(id, v); };

    pageHome->sendNumber = sendNumberCb;
    pageHome->sendString = sendStringCb;

    pageSettings->sendNumber = sendNumberCb;
    pageSettings->sendString = sendStringCb;
}

UI::~UI()
{
}

void UI::receiveNumber(EditId id, float v)
{
    impl_->pageHome_->receiveNumber(id, v);
    impl_->pageSettings_->receiveNumber(id, v);
}

void UI::receiveString(EditId id, cycfi::string_view v)
{
    impl_->pageHome_->receiveString(id, v);
    impl_->pageSettings_->receiveString(id, v);
}
