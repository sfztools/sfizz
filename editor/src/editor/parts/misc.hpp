// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <elements.hpp>
namespace el = cycfi::elements;

///
template <class Element>
auto top_labeled(std::string text, Element&& subject)
{
    return el::vtile(
        el::align_center(el::label(std::move(text))),
        std::forward<Element>(subject));
}
