// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include <elements.hpp>
#include <memory>
#include "PageHome.h"
#include "PageSettings.h"

namespace el = cycfi::elements;

class UI {

public:
    explicit UI(el::view& group);

private:
    // Main window background color
    static constexpr el::color bkd_color = el::rgba(35, 35, 37, 255);
    el::box_element background = el::box(bkd_color);

    std::shared_ptr<PageHome> pageHome;
    std::shared_ptr<PageSettings> pageSettings;
};
