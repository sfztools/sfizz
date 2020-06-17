// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <infra/string_view.hpp>
#include <string>
#include <functional>
#include <cstdarg>

typedef std::function<std::string(double)> value_formatter;

value_formatter create_printf_formatter(const char* format, size_t buffer = 256);
value_formatter create_integer_printf_formatter(const char* format, size_t buffer = 256);
value_formatter create_file_size_formatter();

std::string strprintf(size_t buffer, const char* format, ...);
std::string vstrprintf(size_t buffer, const char* format, va_list ap);
