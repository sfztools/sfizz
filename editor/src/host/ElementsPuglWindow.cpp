// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "PuglHelpers.h"
#include <elements.hpp>
#include <pugl/pugl_cairo.h>
#include <map>
//#include <mutex>

namespace cycfi {
namespace elements {

struct window_info {
    SingletonPuglWorld_s world;
    PuglView_u view;
};

///
static std::map<host_window_handle, std::unique_ptr<window_info>> gWindowInfo;
//static std::mutex gWindowInfoMutex;

static window_info* get_window_info(host_window_handle h)
{
    //std::unique_lock<std::mutex> lock { gWindowInfoMutex };
    auto it = gWindowInfo.find(h);
    return (it != gWindowInfo.end()) ? it->second.get() : nullptr;
}

static void insert_window_info(host_window_handle h, std::unique_ptr<window_info> w)
{
    //std::unique_lock<std::mutex> lock { gWindowInfoMutex };
    gWindowInfo[h] = std::move(w);
}

static void clear_window_info(host_window_handle h)
{
    //std::unique_lock<std::mutex> lock { gWindowInfoMutex };
    gWindowInfo.erase(h);
}

///
window::window(std::string const& name, int style_, rect const& bounds)
{
    std::unique_ptr<window_info> info { new window_info };

    ///
    SingletonPuglWorld_s world = SingletonPuglWorld::instance();
    if (!world)
        return;
    info->world = world;

    PuglView *view = puglNewView(world->get());
    if (!view)
        return;
    info->view.reset(view);
    puglSetHandle(view, this);

    auto eventFunc = +[](PuglView *view, const PuglEvent *event) -> PuglStatus {
        //fprintf(stderr, "Window Event: %d\n", event->type);
        return PUGL_SUCCESS;
    };

    if (puglSetEventFunc(view, eventFunc) != PUGL_SUCCESS)
        return;

    if (puglSetBackend(view, puglCairoBackend()) != PUGL_SUCCESS)
        return;

    if (style_ & window::with_title) {
        if (puglSetWindowTitle(view, name.c_str()) != PUGL_SUCCESS)
            return;
    }

    if (puglSetViewHint(view, PUGL_RESIZABLE, (style_ & window::resizable) != 0) != PUGL_SUCCESS)
        return;

    if (puglSetDefaultSize(view, bounds.width(), bounds.height()) != PUGL_SUCCESS)
        return;

    PuglRect childFrame {};
    childFrame.width = bounds.width();
    childFrame.height = bounds.height();
    if (puglSetFrame(view, childFrame) != PUGL_SUCCESS)
        return;

    if (puglRealize(view) != PUGL_SUCCESS)
        return;

    puglShowWindow(view);

    ///
    host_window_handle h = reinterpret_cast<host_window_handle>(puglGetNativeWindow(view));
    insert_window_info(h, std::move(info));
    _window = h;
}

window::~window()
{
    clear_window_info(_window);
}

point window::size() const
{
    window_info* w = get_window_info(_window);
    if (!w)
        return {};

    PuglRect frame = puglGetFrame(w->view.get());
    return point(frame.width, frame.height);
}

void window::size(point const& p)
{
    window_info* w = get_window_info(_window);
    if (!w)
        return;

    PuglRect frame = puglGetFrame(w->view.get());
    frame.width = p.x;
    frame.height = p.y;
    puglSetFrame(w->view.get(), frame);
}

void window::limits(view_limits limits_)
{
    window_info* w = get_window_info(_window);
    if (!w)
        return;

    puglSetMinSize(w->view.get(), limits_.min.x, limits_.min.y);
    puglSetMaxSize(w->view.get(), limits_.max.x, limits_.max.y);
}

point window::position() const
{
    window_info* w = get_window_info(_window);
    if (!w)
        return {};

    PuglRect rect = puglGetFrame(w->view.get());
    return point(rect.x, rect.y);
}

void window::position(point const& p)
{
    window_info* w = get_window_info(_window);
    if (!w)
        return;

    PuglRect frame = puglGetFrame(w->view.get());
    frame.x = p.x;
    frame.y = p.y;
    puglSetFrame(w->view.get(), frame);
}

} // namespace cycfi
} // namespace elements
