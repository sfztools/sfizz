// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <elements.hpp>
#include <memory>
namespace el = cycfi::elements;

class DemoKnobsAndSliders {
public:
    explicit DemoKnobsAndSliders(el::view& group);

private:
    using slider_ptr = std::shared_ptr<el::basic_slider_base>;
    using dial_ptr = std::shared_ptr<el::dial_base>;

private:
    template <bool is_vertical>
    el::element_ptr make_markers();

    el::element_ptr make_hslider(int index);
    el::element_ptr make_hsliders();

    el::element_ptr make_vslider(int index);
    el::element_ptr make_vsliders();

    el::element_ptr make_dial(int index);
    el::element_ptr make_dials();

    el::element_ptr make_controls();

    void link_control(int index, el::view& view_);

    void link_controls(el::view& view_);

private:
    // Main window background color
    static constexpr el::color bkd_color = el::rgba(35, 35, 37, 255);
    el::box_element background = box(bkd_color);

    slider_ptr hsliders[3];
    slider_ptr vsliders[3];

    dial_ptr dials[3];
};
