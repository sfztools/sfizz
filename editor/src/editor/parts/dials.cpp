// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "dials.hpp"
#include <cmath>
#include <cassert>

size_t multi_choice_dial_base::value() const
{
    double v = dial_->value();
    long n = values_.size();
    long i = std::max(0l, std::min(n - 1, std::lround(v * (n - 1))));
    return i;
}

void multi_choice_dial_base::value(size_t i)
{
    if (i == value())
        return;

    long n = values_.size();
    double v = std::max(0.0, std::min(1.0, static_cast<double>(i) / (n - 1)));
    dial_->value(v);
    update_label_text();
}

void multi_choice_dial_base::update_label_text()
{
    size_t i = value();
    label_->set_text(values_[i]);
}

///
void value_dial_base::formatter(value_formatter f)
{
    format_value_ = std::move(f);
    update_label_text();
}

double value_dial_base::value() const
{
    return range_.denormalize(dial_->value());
}

void value_dial_base::value(double v)
{
    dial_->value(range_.normalize(v));
    update_label_text();
}

void value_dial_base::update_label_text()
{
    std::string text;
    double v = range_.denormalize(dial_->value());
    auto& f = format_value_;
    if (f)
        text = f(v);
    else
        text = std::to_string(v);
    label_->set_text(text);
}
