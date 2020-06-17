// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "formatters.hpp"
#include "ranges.hpp"
#include <elements.hpp>
#include <functional>
namespace el = cycfi::elements;

/**
 * @brief A labeled knob which selects values from a list of strings
 */
struct multi_choice_dial_base : el::proxy_base, el::receiver<size_t> {
public:
    virtual ~multi_choice_dial_base() {}
    //
    size_t value() const override;
    void value(size_t i) override;
    //
    const el::element& subject() const override { return *contents_; }
    el::element& subject() override { return *contents_; }
    //
    void update_label_text();
    //
    std::function<void(size_t)> on_change;
    //
    el::element_ptr contents_;
    std::shared_ptr<el::dial_base> dial_;
    std::shared_ptr<el::label> label_;
    std::vector<std::string> values_;
};

template <class Knob>
struct basic_multi_choice_dial : public multi_choice_dial_base {
};

typedef std::shared_ptr<multi_choice_dial_base> multi_choice_dial_ptr;

template <class Knob>
multi_choice_dial_ptr multi_choice_dial(Knob&& knob, std::vector<std::string> values);

/**
 * @brief A labeled knob which displays its value under it
 */
struct value_dial_base : el::proxy_base, el::receiver<double> {
    virtual ~value_dial_base() {}
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
    std::shared_ptr<el::dial_base> dial_;
    std::shared_ptr<el::label> label_;
    value_range range_;
    value_formatter format_value_ = create_printf_formatter("%g");
};

template <class Knob>
struct basic_value_dial : public value_dial_base {
};

typedef std::shared_ptr<value_dial_base> value_dial_ptr;

template <class Knob>
value_dial_ptr value_dial(Knob&& knob, value_range range);

#include "dials.tcc"
