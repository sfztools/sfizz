// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Knob.h"

Knob::Knob(el::view& view_, const std::string& lbl, Type type, double value)
    : parentView_(view_)
    , label_(lbl)
    , labelValue_(el::share(el::label("")))
    , type_(type)
    , value_(value)
{
    float const dialScale = 1.0 / 4;
    el::sprite knob = el::sprite { "knob.png", 128 * dialScale, dialScale };
    dial_ = el::share(el::dial(knob, value_));

    contents_ = el::share(
        el::vtile(
            el::align_center(label_),
            el::align_center(el::hold(dial_)),
            el::align_center(el::hold(labelValue_))));

    setValue_(value_);
    parentView_.refresh(*labelValue_);

    dial_->on_change = [this](double val) {
        setValue_(val);
    };
}
el::element_ptr Knob::contents() const
{
    return contents_;
}
void Knob::setValue_(double val)
{
    std::string sVal;

    if (type_ == Type::Polyphony) {
        int8_t i = val * 6 + 3;
        sVal = std::to_string(1 << i) + " Voices";
    } else if (type_ == Type::Oversampling) {
        int8_t i = val * 3;
        sVal = "x" + std::to_string(1 << i);
    } else {
        int8_t i = val * 6 + 2;
        sVal = std::to_string(1 << i) + " KB";
    }
    labelValue_->set_text(sVal);
    parentView_.refresh();
}
