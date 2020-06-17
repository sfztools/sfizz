// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "sliders.hpp"
#include <cmath>

void value_slider_base::formatter(value_formatter f)
{
    format_value_ = std::move(f);
    update_label_text();
}

double value_slider_base::value() const
{
    return range_.denormalize(slider_->value());
}

void value_slider_base::value(double v)
{
    slider_->value(range_.normalize(v));
    update_label_text();
}

void value_slider_base::update_label_text()
{
    std::string text;
    double v = range_.denormalize(slider_->value());
    auto& f = format_value_;
    if (f)
        text = f(v);
    else
        text = std::to_string(v);
    label_->set_text(text);
}
