// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "UI.h"
#include "Editor.h"

constexpr el::color UI::bkd_color;

UI::UI(el::view& group)
{
    group.content(
        el::hmin_size(Editor::fixedWidth,
            el::vmin_size(Editor::fixedHeight,
                el::vtile(
                    el::hold(make_toolbar(group))

#if HAVE_ELEMENTS_NOTEBOOK
                        ,
                    el::top_margin(95, make_tabs(group)),
#endif
                    ))),
        background);
}
el::basic_menu UI::make_pbrange_menu(const char* title, el::menu_position pos)
{
    auto popup = el::button_menu(title, pos);
    auto menu = el::layer(
        el::vtile(
            el::menu_item("Default"),
            el::menu_item("1"),
            el::menu_item("2"),
            el::menu_item("3"),
            el::menu_item("4"),
            el::menu_item("5"),
            el::menu_item("6"),
            el::menu_item("7"),
            el::menu_item("12"),
            el::menu_item("24")),
        el::panel {});
    popup.menu(el::hsize(300, menu));

    return popup;
}
el::basic_menu UI::make_polyphony_menu(const char* title, el::menu_position pos)
{
    auto popup = el::button_menu(title, pos);
    auto menu = el::layer(
        el::vtile(
            el::menu_item("1"),
            el::menu_item("2"),
            el::menu_item("8"),
            el::menu_item("16"),
            el::menu_item("32"),
            el::menu_item("64"),
            el::menu_item("128")),
        el::panel {});
    popup.menu(el::hsize(300, menu));

    return popup;
}
el::element_ptr UI::make_toolbar(el::view& view_)
{
    dialVolume = std::make_shared<Dial>(view_, "Volume", 0.75f);
    dialPan = std::make_shared<Dial>(view_, "Pan", 0.5f, Dial::Pan);
    dialSend = std::make_shared<Dial>(view_, "Send");
    dialTune = std::make_shared<Dial>(view_, "Tune", 0.5f, Dial::Tune);
    dialTranspose = std::make_shared<Dial>(view_, "Transpose", 0.5f, Dial::Transpose);

    return el::share(el::layer(
        el::top_margin(4,
            el::htile(
                el::hold(dialVolume->contents()),
                el::hold(dialPan->contents()),
                el::hold(dialSend->contents()),
                el::hold(dialTune->contents()),
                el::hold(dialTranspose->contents()),
                el::vtile(
                    el::top_margin(4, make_polyphony_menu("Polyphony", el::menu_position::bottom_right)),
                    el::top_margin(4, make_pbrange_menu("PitchBend Range", el::menu_position::bottom_right)))))));
}
#if HAVE_ELEMENTS_NOTEBOOK
el::element_ptr UI::make_tabs(el::view& view_)
{
    auto make_page_info = []() {
        return el::layer(
            el::align_center_middle(el::image("logo.png")),
            el::hstretch(0.5, el::image("bg.png")),
            el::frame {});
    };
    return el::notebook(
        view_,
        el::deck(
            make_page_info(),
            background,
            background),
        el::tab("Info"),
        el::tab("Controls"),
        el::tab("Settings"));
}
#endif
