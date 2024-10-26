// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <functional>
#include <ghc/fs_std.hpp>

namespace sfz {

class FileOpenPreexec {
public:
    using ChildFunction = std::function<void(const fs::path&)>;
    using HandlerFunction = std::function<bool(const fs::path&, ChildFunction&)>;

    FileOpenPreexec() :
    handler([](const fs::path& path, ChildFunction func) -> bool {
        func(path);
        return true;
    })
    {}

    void setHandler(HandlerFunction handlerFunction)
    {
        if (handlerFunction) {
            handler = handlerFunction;
        }
        else {
            handler = [](const fs::path& path, ChildFunction& func) -> bool {
                func(path);
                return true;
            };
        }
    }

    bool executeFileOpen(const fs::path& path, ChildFunction function)
    {
        return handler(path, function);
    }

private:
    HandlerFunction handler;
};

}
