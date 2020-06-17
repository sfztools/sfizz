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
                        el::deck(el::hold(pageHome),
                            el::hold(pageSettings)),
                        el::tab("Home"), el::tab("Settings"))))),
        el::box(bg_color));

    pageHome->on_change_preload_size = [this](int v) {
        impl_->ctrl_->uiSendNumber(EditId::PreloadSize, v);
    };
    pageHome->on_change_volume = [this](double v) {
        impl_->ctrl_->uiSendNumber(EditId::Volume, v);
    };
    pageHome->on_change_polyphony = [this](int v) {
        impl_->ctrl_->uiSendNumber(EditId::Polyphony, v);
    };
    pageHome->on_change_oversampling = [this](int v) {
        impl_->ctrl_->uiSendNumber(EditId::Oversampling, v);
    };

    pageHome->on_change_sfz_file = [this](absl::string_view v) {
        impl_->ctrl_->uiSendString(EditId::SfzFile, v);
    };
    pageSettings->on_change_scala_file = [this](absl::string_view v) {
        impl_->ctrl_->uiSendString(EditId::ScalaFile, v);
    };
    pageSettings->on_change_scala_root_key = [this](int v) {
        impl_->ctrl_->uiSendNumber(EditId::ScalaRootKey, v);
    };
    pageSettings->on_change_tuning_frequency = [this](double v) {
        impl_->ctrl_->uiSendNumber(EditId::TuningFrequency, v);
    };
    pageSettings->on_change_stretch_tuning = [this](double v) {
        impl_->ctrl_->uiSendNumber(EditId::StretchTuning, v);
    };
}

UI::~UI()
{
}

void UI::updatePreloadSize(int v)
{
    impl_->pageHome_->updatePreloadSize(v);
}

void UI::updateVolume(float v)
{
    impl_->pageHome_->updateVolume(v);
}

void UI::updatePolyphony(float v)
{
    impl_->pageHome_->updatePolyphony(v);
}

void UI::updateOversampling(int v)
{
    impl_->pageHome_->updateOversampling(v);
}

void UI::updateSfzFile(cycfi::string_view v)
{
    impl_->pageHome_->updateSfzFile(v);
}

void UI::updateScalaFile(cycfi::string_view v)
{
    impl_->pageSettings_->updateScalaFile(v);
}

void UI::updateScalaRootKey(float v)
{
    impl_->pageSettings_->updateScalaRootKey(v);
}

void UI::updateTuningFrequency(float v)
{
    impl_->pageSettings_->updateTuningFrequency(v);
}

void UI::updateStretchTuning(float v)
{
    impl_->pageSettings_->updateStretchTuning(v);
}
