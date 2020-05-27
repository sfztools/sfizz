// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <memory>

class NativeIdleRunner {
public:
    NativeIdleRunner();
    ~NativeIdleRunner();

    void start(double interval, void(*cbfn)(void*), void* cbdata);
    void stop();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
