// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <string>
#include <vector>
#include <new>
#include <stdexcept>

class EditValue {
public:
    constexpr EditValue() : tag(Nil) {}
    EditValue(float value) { reset(value); }
    EditValue(std::vector<float> value) { reset(std::move(value)); }
    EditValue(std::string value) { reset(std::move(value)); }
    ~EditValue() { reset(); }

    void reset() noexcept
    {
        if (tag == String)
            destruct(u.s);
        else if (tag == FloatVector)
            destruct(u.v);
        tag = Nil;
    }

    void reset(float value) noexcept
    {
        reset();
        u.f = value;
        tag = Float;
    }

    void reset(std::vector<float> value) noexcept
    {
        reset();
        new (&u.v) std::vector<float>(std::move(value));
        tag = FloatVector;
    }

    void reset(std::string value) noexcept
    {
        reset();
        new (&u.s) std::string(std::move(value));
        tag = String;
    }

    float to_float() const
    {
        if (tag != Float)
            throw std::runtime_error("the tagged union does not contain `float`");
        return u.f;
    }

    const std::vector<float>& to_float_vector() const
    {
        if (tag != FloatVector)
            throw std::runtime_error("the tagged union does not contain `vector<float>`");
        return u.v;
    }

    const std::string& to_string() const
    {
        if (tag != String)
            throw std::runtime_error("the tagged union does not contain `string`");
        return u.s;
    }

private:
    template <class T> static void destruct(T& obj) { obj.~T(); }

private:
    enum TypeTag { Nil, Float, FloatVector, String };
    union Union {
        constexpr explicit Union(float f = 0.0f) noexcept : f(f) {}
        ~Union() noexcept {}
        float f;
        std::vector<float> v;
        std::string s;
    };
    TypeTag tag { Nil };
    Union u;
};
