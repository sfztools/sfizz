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
