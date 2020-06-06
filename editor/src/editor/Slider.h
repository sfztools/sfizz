// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include <elements.hpp>

namespace el = cycfi::elements;

class Slider {

public:
    Slider() = delete;

    explicit Slider(el::view& view_, const std::string& lbl, double value = 1.0f);

    el::element_ptr contents() const;

private:
    void setValue_(double val);

    el::view& parentView_;
    el::label label_;
    std::shared_ptr<el::label> labelValue_;
    std::shared_ptr<el::basic_slider_base> slider_;
    double value_;
    el::element_ptr contents_;
};
