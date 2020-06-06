// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PageHome.h"
#include <combobox.hpp>

#include <memory>
#include <functional>

PageHome::PageHome(el::view& view_)
    : knbPolyphony(std::make_shared<Knob>(view_, "Polyphony", Knob::Type::Polyphony, 0.85))
    , knbOversampling(std::make_shared<Knob>(view_, "Oversampling", Knob::Type::Oversampling, 0))
    , knbPreload(std::make_shared<Knob>(view_, "Preload", Knob::Type::PreloadSize, 0.58))
    , sldVolume(std::make_shared<Slider>(view_, "Volume", 0.75f))
{
    auto ibSfz = el::input_box("Select a sfz file...");
    auto ibScala = el::input_box("Select a scala file...");

    txtSfz = ibSfz.second;
    txtScala = ibScala.second;

    el::layered_button btnSfz = el::button("...");
    el::layered_button btnScala = el::button("...");

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
    btnScala.on_click =
        [this](bool) mutable {
            fileDialog.reset(new FileDialog);
            fileDialog->onFileChosen = [this](absl::string_view fileName) {
                if (fileName != "" && fileName != txtScala->get_text()) {
                    txtScala->set_text(std::string(fileName));
                    // TODO: Set scala file in plugin
                }
            };
            fileDialog->setMode(FileDialog::Mode::Open);
            fileDialog->setTitle("Open a scala file...");
            fileDialog->addFilter(FileDialog::Filter { "Scala Files", { "*.scl", "*.SCL" } });
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
                                el::group("Scala Tuning",
                                    el::margin({ 10, 10, 10, 10 },
                                        el::top_margin(26,
                                            el::vtile(
                                                el::htile(
                                                    ibScala.first,
                                                    el::left_margin(10,
                                                        el::hsize(30,
                                                            btnScala))),
                                                el::top_margin(10,
                                                    el::htile(
                                                        makeScalaCenterMenu(),
                                                        el::left_margin(10,
                                                            makeScalaTuningMenu())))))))),
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
el::basic_menu PageHome::makeScalaCenterMenu()
{
    auto popup = combo_box(
        [](cycfi::string_view /*select*/) {
            // This will be called when an item is selected
        },
        { "A1", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4",
            "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5",
            "A5", "A#5", "B5", "C6" })
                     .first;
    return popup;
}
el::basic_menu PageHome::makeScalaTuningMenu()
{
    auto popup = combo_box(
        [](cycfi::string_view /*select*/) {
            // This will be called when an item is selected
        },
        { "English pitchpipe 380 (1720)",
            "Handel fork1 409 (1780)",
            "Baroque 415",
            "Handel fork 422.5 (1740)",
            "Dresden opera 423.2 (1815)",
            "French Law 435 (1859)",
            "British Phil 439 (1896)",
            "International 440",
            "European 442",
            "Germany, China 445",
            "La Scala in Milan 451 (18th)" })
                     .first;
    return popup;
}
