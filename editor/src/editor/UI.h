// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <elements.hpp>
#include <memory>
#include <functional>
class EditorController;
enum class EditId : int;

namespace el = cycfi::elements;

class UI {
public:
    UI(el::view& group, EditorController& ctrl);
    ~UI();

    void receiveNumber(EditId id, float v);
    void receiveString(EditId id, cycfi::string_view v);
    std::function<void(EditId, float)> sendNumber = [](EditId, float) {};
    std::function<void(EditId, cycfi::string_view)> sendString = [](EditId, cycfi::string_view) {};

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
