// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "sliders.hpp"

template <class Thumb, class Track>
value_slider_ptr value_slider(Thumb&& thumb, Track&& track, value_range range)
{
    assert(!values.empty());
    auto vd = std::make_shared<basic_value_slider<Thumb, Track>>();

    auto slider = el::share(el::slider(std::move(thumb), std::move(track)));
    auto label = el::share(el::label(std::string()));
    vd->slider_ = slider;
    vd->label_ = label;
    vd->range_ = range;

    slider->on_change = [vd](double v_)
    {
        std::string text;
        double v = vd->range_.denormalize(v_);
        vd->update_label_text();
        if (vd->on_change)
            vd->on_change(v);
    };

    slider->on_change(vd->slider_->value());

    vd->contents_ = el::share(el::vtile(
        el::align_center(el::hold(slider)),
        el::align_center(el::hold(label))));

    return vd;
}
