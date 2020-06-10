// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PageSettings.h"

#include <combobox.hpp>

PageSettings::PageSettings(el::view& view_)
{
    auto ibScala = el::input_box(""); // FIXME: "Select a scala file..." inputbox is messed up
    txtScala = ibScala.second;

    el::layered_button btnScala = el::button("...");

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
                el::margin({ 10, 10, 10, 10 },
                    el::vtile(
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
                                                    makeScalaTuningMenu())))))))))));
}
el::element_ptr PageSettings::contents() const
{
    return contents_;
}
el::basic_menu PageSettings::makeScalaCenterMenu()
{
    auto popup = sfizz::combo_box(
        [](cycfi::string_view /*select*/) {
            // This will be called when an item is selected
        },
        { "A1", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4",
            "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5",
            "A5", "A#5", "B5", "C6" })
                     .first;
    return popup;
}
el::basic_menu PageSettings::makeScalaTuningMenu()
{
    auto popup = sfizz::combo_box(
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
