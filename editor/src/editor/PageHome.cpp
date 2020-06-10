// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PageHome.h"

#include <memory>
#include <functional>

PageHome::PageHome(el::view& view_)
    : knbPolyphony(std::make_shared<Knob>(view_, "Polyphony", Knob::Type::Polyphony, 0.85))
    , knbOversampling(std::make_shared<Knob>(view_, "Oversampling", Knob::Type::Oversampling, 0))
    , knbPreload(std::make_shared<Knob>(view_, "Preload", Knob::Type::PreloadSize, 0.58))
    , sldVolume(std::make_shared<Slider>(view_, "Volume", 0.75f))
{
    auto ibSfz = el::input_box(""); // FIXME: "Select a sfz file..." inputbox is messed up
    txtSfz = ibSfz.second;

    el::layered_button btnSfz = el::button("...");

    btnSfz.on_click =
        [this](bool) mutable {
            fileDialog.reset(new FileDialog);
            fileDialog->onFileChosen = [this](absl::string_view fileName) {
                if (fileName != "" && fileName != txtSfz->get_text()) {
                    txtSfz->set_text(std::string(fileName));
                    // TODO: Set scala file in plugin
                }
            };
            fileDialog->setMode(FileDialog::Mode::Open);
            fileDialog->setTitle("Open a sfz file...");
            fileDialog->addFilter(FileDialog::Filter { "SFZ Files", { "*.sfz", "*.SFZ" } });
            fileDialog->chooseFile();
            fileDialog.reset();
        };
    contents_
        = el::share(
            el::layer(
                el::htile(
                    el::margin({ 10, 10, 10, 10 },
                        el::vtile(
                            el::group("SFZ File",
                                el::margin({ 10, 10, 10, 10 },
                                    el::top_margin(26,
                                        el::htile(
                                            ibSfz.first,
                                            el::left_margin(10,
                                                el::hsize(30,
                                                    btnSfz)))))),
                            el::top_margin(10,
                                el::htile(
                                    el::hold(knbPolyphony->contents()),
                                    el::hold(knbOversampling->contents()),
                                    el::hold(knbPreload->contents()))))),
                    el::margin({ 0, 10, 10, 10 },
                        el::hold(sldVolume->contents())))));
}
el::element_ptr PageHome::contents() const
{
    return contents_;
}
