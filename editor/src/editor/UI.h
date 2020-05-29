// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include <elements.hpp>
#include <memory>
#include "Dial.h"

namespace el = cycfi::elements;

class UI {

public:
    explicit UI(el::view& group);

private:
    el::basic_menu make_pbrange_menu(const char* title, el::menu_position pos);
    el::basic_menu make_polyphony_menu(const char* title, el::menu_position pos);
    el::element_ptr make_toolbar(el::view& view_);

#if HAVE_ELEMENTS_NOTEBOOK
    el::element_ptr make_tabs(el::view& view_);
#endif
    using slider_ptr = std::shared_ptr<el::basic_slider_base>;
    using dial_ptr = std::shared_ptr<Dial>;

    // Main window background color
    static constexpr el::color bkd_color = el::rgba(35, 35, 37, 255);
    el::box_element background = el::box(bkd_color);

    dial_ptr dialVolume;
    dial_ptr dialPan;
    dial_ptr dialSend;
    dial_ptr dialTune;
    dial_ptr dialTranspose;
};
