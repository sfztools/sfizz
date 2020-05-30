// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Knob.h"
#include "Res.h"

#define USE_ELEMENTS_RESOURCES 1

Knob::Knob(el::view& view_, const std::string& lbl, double value, Type type)
    : parentView_(view_)
    , label_(el::share(el::label(lbl)))
    , labelValue_(el::share(el::label("")))
    , value_(value)
    , type_(type)
{
#if USE_ELEMENTS_RESOURCES
    float const knob_scale = 1.0 / 4;
    el::sprite knob = el::sprite { "knob.png", 128 * knob_scale, knob_scale };
    dial_ = el::share(el::dial(
        el::radial_marks<15>(knob), value_));
#else
    dial_ = el::share(el::dial(
        el::radial_marks<15>(el::basic_knob<40>()), value_));
#endif
    contents_ = el::share(
        el::fixed_size({ 60, 60 },
            el::vtile(
                el::align_center(el::hold(label_)),
                el::align_center(el::hold(dial_)),
                el::align_center(el::hold(labelValue_)))));

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
    char sVal[16];

    if (type_ == Type::Cents) {
        int8_t i = val * 100;
        std::sprintf(sVal, "%i%%", i);

    } else if (type_ == Type::Pan) {
        int8_t i = (val * 2 - 1) * 100;
        if (i < 0)
            std::sprintf(sVal, "%i%% L", i);
        else if (i > 0)
            std::sprintf(sVal, "%i%% R", i);
        else
            std::sprintf(sVal, "Center");

    } else if (type_ == Type::Transpose) {
        int8_t i = (val * 2 - 1) * 12;
        std::sprintf(sVal, "%i", i);

    } else if (type_ == Type::Tune) {
        int8_t i = (val * 2 - 1) * 100;
        std::sprintf(sVal, "%i", i);
    }
    labelValue_->set_text(sVal);
    parentView_.refresh();
}
