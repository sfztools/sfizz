// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include <elements.hpp>

namespace el = cycfi::elements;

class Dial {

public:
    enum Type {
        Cents = 1,
        Pan,
        Transpose,
        Tune
    };
    Dial() = delete;

    explicit Dial(el::view& view_, const std::string& lbl,
        double value = 1.0f, Type type = Dial::Cents);

    el::element_ptr contents() const;

private:
    using dialPtr = std::shared_ptr<el::dial_base>;
    using labelPtr = std::shared_ptr<el::label>;

    void setValue_(double val);

    el::view& parentView_;
    dialPtr dial_;
    labelPtr label_;
    labelPtr labelValue_;
    double value_;
    Type type_;
    el::element_ptr contents_;
};
