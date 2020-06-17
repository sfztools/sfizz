// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "dials.hpp"

template <class Knob>
multi_choice_dial_ptr multi_choice_dial(Knob&& knob, std::vector<std::string> values)
{
    assert(!values.empty());
    auto multi = std::make_shared<basic_multi_choice_dial<Knob>>();

    auto dial = el::share(el::dial(std::move(knob)));
    auto label = el::share(el::label(values.front()));
    multi->dial_ = dial;
    multi->label_ = label;
    multi->values_ = std::move(values);

    dial->on_change = [multi](double v)
    {
        long n = multi->values_.size();
        long i = std::max(0l, std::min(n - 1, std::lround(v * (n - 1))));
        multi->update_label_text();
        if (multi->on_change)
            multi->on_change(i);
    };

    multi->contents_ = el::share(el::vtile(
        el::align_center(el::hold(dial)),
        el::align_center(el::hold(label))));

    return multi;
}

template <class Knob>
value_dial_ptr value_dial(Knob&& knob, value_range range)
{
    assert(!values.empty());
    auto vd = std::make_shared<basic_value_dial<Knob>>();

    auto dial = el::share(el::dial(std::move(knob)));
    auto label = el::share(el::label(std::string()));
    vd->dial_ = dial;
    vd->label_ = label;
    vd->range_ = range;

    vd->dial_->on_change = [vd](double v_)
    {
        double v = vd->range_.denormalize(v_);
        vd->update_label_text();
        if (vd->on_change)
            vd->on_change(v);
    };

    vd->dial_->on_change(vd->dial_->value());

    vd->contents_ = el::share(el::vtile(
        el::align_center(el::hold(vd->dial_)),
        el::align_center(el::hold(vd->label_))));

    return vd;
}
