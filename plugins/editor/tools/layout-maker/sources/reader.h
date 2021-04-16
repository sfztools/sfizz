// -*- C++ -*-
// SPDX-License-Identifier: BSL-1.0
//
//          Copyright Jean Pierre Cimalando 2019-2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#include "layout.h"
#include <string>
#include <stdexcept>

Layout read_file_layout(const char *filename);

///
struct file_format_error : public std::runtime_error {
public:
    explicit file_format_error(const std::string &reason = "Format error")
        : runtime_error(reason) {}
};
