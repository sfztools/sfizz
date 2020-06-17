// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "formatters.hpp"
#include "ranges.hpp"
#include <elements.hpp>
namespace el = cycfi::elements;

/**
 * @brief A labeled slider which displays its value under it
 */
struct value_slider_base : el::proxy_base, el::receiver<double> {
    virtual ~value_slider_base() {}
    void formatter(value_formatter f);
    std::function<void(double)> on_change;
    //
    const el::element& subject() const override { return *contents_; }
    el::element& subject() override { return *contents_; }
    //
    double value() const override;
    void value(double v) override;
    //
    void update_label_text();
    //
    el::element_ptr contents_;
    std::shared_ptr<el::slider_base> slider_;
    std::shared_ptr<el::label> label_;
    value_range range_;
    value_formatter format_value_ = create_printf_formatter("%g");
};

template <class Thumb, class Track>
struct basic_value_slider : public value_slider_base {
};

typedef std::shared_ptr<value_slider_base> value_slider_ptr;

template <class Thumb, class Track>
value_slider_ptr value_slider(Thumb&& thumb, Track&& track, value_range range);

#include "sliders.tcc"
