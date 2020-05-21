// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include <pugl/pugl.h>
#include <memory>

struct PuglViewDeleter {
    void operator()(PuglView *x) const { puglFreeView(x); }
};
typedef std::unique_ptr<PuglView, PuglViewDeleter> PuglView_u;

struct SingletonPuglWorld {
public:
    static std::shared_ptr<SingletonPuglWorld> instance();

    PuglWorld* get() const { return world_.get(); }

private:
    SingletonPuglWorld() {}

private:
    struct Deleter {
        void operator()(PuglWorld *x) const { puglFreeWorld(x); }
    };
    std::unique_ptr<PuglWorld, Deleter> world_;
};
typedef std::shared_ptr<SingletonPuglWorld> SingletonPuglWorld_s;
