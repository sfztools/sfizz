// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "Editor.h"
#include "Res.h"
#include "Demo.h"
#include <elements.hpp>
#include <elements/support/resource_paths.hpp>
#include <algorithm>
namespace el = cycfi::elements;

namespace cycfi {
namespace elements {

void show_window(base_view& w);
void hide_window(base_view& w);
void process_events(base_view& w);

} // namespace elements
} // namespace cycfi

constexpr int Editor::fixedWidth;
constexpr int Editor::fixedHeight;

struct Editor::Impl {
    std::unique_ptr<el::view> view_;

    //--- TODO UI: from sliders and knobs example
    std::unique_ptr<DemoKnobsAndSliders> demo_;
};

///

Editor::Editor()
    : impl_(new Impl)
{
}

Editor::~Editor()
{
}

bool Editor::open(void* parentWindowId)
{
    el::view* view = new el::view(parentWindowId);
    impl_->view_.reset(view);

    if (!view->host()) {
        impl_->view_.reset();
        return false;
    }

    view->size({fixedWidth, fixedHeight});

    // make the resource path known to Elements
    cycfi::fs::path resourcePath = Res::getRootPath();
    if (!resourcePath.empty()) {
        auto it = std::find(el::resource_paths.begin(), el::resource_paths.end(), resourcePath);
        if (it == el::resource_paths.end())
            el::resource_paths.push_back(resourcePath);
    }

    ///
    DemoKnobsAndSliders* demo = new DemoKnobsAndSliders(*view);
    impl_->demo_.reset(demo);

    ///
    return true;
}

void Editor::close()
{
    impl_->view_.reset();
}

bool Editor::isOpen() const
{
    return impl_->view_ != nullptr;
}

void Editor::show()
{
    if (impl_->view_)
        el::show_window(*impl_->view_);
}

void Editor::hide()
{
    if (impl_->view_)
        el::hide_window(*impl_->view_);
}

void Editor::processEvents()
{
    if (impl_->view_)
        el::process_events(*impl_->view_);
}
