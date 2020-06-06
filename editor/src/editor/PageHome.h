// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once

#include <elements.hpp>
#include "Knob.h"
#include "Slider.h"
#include "native/FileDialog.h"

namespace el = cycfi::elements;

class PageHome {

public:
    PageHome(el::view& view_);

    el::element_ptr contents() const;

private:
    el::basic_menu makeScalaCenterMenu();
    el::basic_menu makeScalaTuningMenu();

    el::element_ptr contents_;

    std::shared_ptr<Knob> knbPolyphony;
    std::shared_ptr<Knob> knbOversampling;
    std::shared_ptr<Knob> knbPreload;

    std::shared_ptr<el::basic_input_box> txtScala;
    std::shared_ptr<el::basic_input_box> txtSfz;

    std::shared_ptr<Slider> sldVolume;

    std::shared_ptr<FileDialog> fileDialog;
};
