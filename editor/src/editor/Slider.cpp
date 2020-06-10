// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Slider.h"

Slider::Slider(el::view& view_, const std::string& lbl, double value)
    : parentView_(view_)
    , label_(lbl)
    , labelValue_(el::share(el::label("")))
    , value_(value)
{
    el::image slider_knob = el::image { "slider-v.png", 1.0 / 4 };
    slider_ = el::share(el::slider(
        el::align_center(slider_knob), el::slider_marks<30>(el::basic_track<4, true>()), value_));

    contents_ = el::share(
        el::vmin_size(240,
            el::vtile(
                label_,
                el::hold(slider_),
                el::hold(labelValue_))));

    setValue_(value_);
    parentView_.refresh(*labelValue_);

    slider_->on_change = [this](double val) {
        setValue_(val);
        // TODO: Set volume in plugin
    };
}
el::element_ptr Slider::contents() const
{
    return contents_;
}
void Slider::setValue_(double val)
{
    char sVal[16];
    int8_t i = val * 100;
    std::sprintf(sVal, "%i%%", i);
    labelValue_->set_text(sVal);
    parentView_.refresh();
}
