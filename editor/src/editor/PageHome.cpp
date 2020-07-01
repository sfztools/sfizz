// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PageHome.h"
#include "EditIds.h"
#include "parts/dials.hpp"
#include "parts/sliders.hpp"
#include "parts/misc.hpp"
#include "native/FileDialog.h"

struct PageHome::Impl {
    PageHome* self_ = nullptr;

    void init(el::view& view);
    void askToChooseSfzFile();

    el::element_ptr contents_;

    value_dial_ptr knbPolyphony_;
    multi_choice_dial_ptr knbOversampling_;
    value_dial_ptr knbPreload_;
    value_slider_ptr sldVolume_;
    std::shared_ptr<FileDialog> fileDialog_;
    std::shared_ptr<el::basic_input_box> txtSfz_;
};

PageHome::PageHome(el::view& view)
    : impl_(new Impl)
{
    impl_->self_ = this;
    impl_->init(view);
}

PageHome::~PageHome()
{
}

const el::element& PageHome::subject() const
{
    return *impl_->contents_;
}

el::element& PageHome::subject()
{
    return *impl_->contents_;
}

void PageHome::receiveNumber(EditId id, float v)
{
    switch (id) {
    case EditId::Volume:
        impl_->sldVolume_->value(v);
        break;
    case EditId::Polyphony:
        impl_->knbPolyphony_->value(v);
        break;
    case EditId::Oversampling:
        {
            // convert UI value using integer log2
            int i = 0;
            for (int f = std::max<int>(1, v); f > 1; f /= 2)
                ++i;
            impl_->knbOversampling_->value(i);
        }
        break;
    case EditId::PreloadSize:
        impl_->knbPreload_->value(v);
        break;
    default:
        break;
    }
}

void PageHome::receiveString(EditId id, cycfi::string_view v)
{
    switch (id) {
    case EditId::SfzFile:
        impl_->txtSfz_->value(v);
        break;
    default:
        break;
    }
}

void PageHome::Impl::init(el::view& view)
{
    const float dialScale = 1.0 / 4;
    auto dialSprite = el::share(el::sprite { "knob.png", 128 * dialScale, dialScale });
    auto thumbImg = el::share(el::image { "slider-v.png", 1.0 / 4 });

    knbPolyphony_ = value_dial(el::hold(dialSprite), { 8, 256 });
    knbOversampling_ = multi_choice_dial(el::hold(dialSprite), {"1x", "2x", "4x", "8x"});
    knbPreload_ = value_dial(el::hold(dialSprite), { 1024, 65536 });
    sldVolume_ = value_slider(
        el::align_center(el::hold(thumbImg)),
        el::slider_marks<30>(el::basic_track<4, true>()),
        { -80.0, 6.0 });

    knbPolyphony_->formatter(create_integer_printf_formatter("%d voices"));
    knbPreload_->formatter(create_file_size_formatter());
    sldVolume_->formatter(create_printf_formatter("%.1f dB"));

    knbPreload_->on_change = [this](double v) { self_->sendNumber(EditId::PreloadSize, static_cast<int>(v)); };
    sldVolume_->on_change = [this](double v) { self_->sendNumber(EditId::Volume, v); };
    knbPolyphony_->on_change = [this](double v) { self_->sendNumber(EditId::Polyphony, static_cast<int>(v)); };
    knbOversampling_->on_change = [this](double v) { self_->sendNumber(EditId::Oversampling, 1 << static_cast<int>(v)); };

    auto ibSfz = el::input_box(""); // FIXME: "Select a sfz file..." inputbox is messed up
    txtSfz_ = ibSfz.second;

    el::layered_button btnSfz = el::button("...");

    btnSfz.on_click = [this](bool) { askToChooseSfzFile(); };

    auto sfzGroup = el::group("SFZ File",
                                el::margin({ 10, 10, 10, 10 },
                                    el::top_margin(26,
                                        el::htile(
                                            ibSfz.first,
                                            el::left_margin(10,
                                                el::hsize(30,
                                                    btnSfz))))));

    contents_
        = el::share(
            el::layer(
                el::htile(
                    el::margin({ 10, 10, 10, 10 },
                        el::vtile(
                            std::move(sfzGroup),
                            el::top_margin(10,
                                el::htile(
                                    top_labeled("Polyphony",
                                        el::hold(knbPolyphony_)),
                                    top_labeled("Oversampling",
                                        el::hold(knbOversampling_)),
                                    top_labeled("Preload size",
                                        el::hold(knbPreload_)))))),
                    el::margin({ 0, 10, 10, 10 },
                        top_labeled("Volume",
                            el::hold(sldVolume_))))));
}

void PageHome::Impl::askToChooseSfzFile()
{
    FileDialog* dlg = fileDialog_.get();
    if (dlg)
        return;

    dlg = new FileDialog;
    fileDialog_.reset(dlg);

    dlg->setMode(FileDialog::Mode::Open);
    dlg->setTitle("Open a sfz file...");
    dlg->addFilter(FileDialog::Filter { "SFZ Files", { "*.sfz" } });

    dlg->onFileChosen = [this](absl::string_view fileName_) {
         if (!fileName_.empty()) {
             std::string fileName(fileName_);
             txtSfz_->set_text(fileName);
             self_->sendString(EditId::SfzFile, fileName);
         }
         fileDialog_.reset();
    };

    if (!dlg->chooseFile())
        fileDialog_.reset();
}
