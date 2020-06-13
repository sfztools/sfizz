// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <elements.hpp>
#include <functional>

namespace el = cycfi::elements;

class Knob {

public:
    enum Type {
        Polyphony = 1,
        Oversampling,
        PreloadSize
    };
    Knob() = delete;

    explicit Knob(el::view& view_, const std::string& lbl, Type type,
        double value = 1.0f);

    el::element_ptr contents() const;

    std::function<void(double)> on_change;

private:
    void setValue_(double val);

    el::view& parentView_;
    el::label label_;

    std::shared_ptr<el::dial_base> dial_;
    std::shared_ptr<el::label> labelValue_;
    el::element_ptr contents_;

    Type type_;
    double value_;
};
