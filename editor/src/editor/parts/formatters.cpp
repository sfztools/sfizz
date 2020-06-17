// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "formatters.hpp"
#include <infra/string_view.hpp>
#include <memory>
#include <cstdio>

value_formatter create_printf_formatter(const char* format, size_t buffer)
{
    return [format, buffer](double v) -> std::string {
        return strprintf(buffer, format, v);
    };
}

value_formatter create_integer_printf_formatter(const char* format, size_t buffer)
{
    return [format, buffer](double v) -> std::string {
        return strprintf(buffer, format, static_cast<int>(v));
    };
}

value_formatter create_file_size_formatter()
{
    return [](double v) -> std::string {
        size_t size = static_cast<long>((v < 0.0) ? 0.0 : v);
        cycfi::string_view unit = " B";
        if (size >= 1024) {
            size /= 1024;
            unit = " kB";
            if (size >= 1024) {
                size /= 1024;
                unit = " MB";
                if (size >= 1024) {
                    size /= 1024;
                    unit = " GB";
                }
            }
        }
        return std::to_string(size) + std::string(unit);
    };
}

std::string strprintf(size_t buffer, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    std::string str = vstrprintf(buffer, format, ap);
    va_end(ap);
    return str;
}

std::string vstrprintf(size_t buffer, const char* format, va_list ap)
{
    std::unique_ptr<char[]> text { new char[buffer + 1] };
    vsnprintf(text.get(), buffer, format, ap);
    text[buffer] = '\0';
    return text.get();
}
