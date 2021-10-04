// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#if !defined(__APPLE__) && !defined(_WIN32)
#include "X11RunLoop.h"
#include "vstgui/lib/platform/linux/x11platform.h"
#include "base/source/fobject.h"
#include <vector>
#include <typeinfo>
#include <cstdio>
#include <cassert>

namespace VSTGUI {

struct RunLoop::Impl {
    struct EventHandler;
    struct TimerHandler;

    using EventHandlers = std::vector<Steinberg::IPtr<EventHandler>>;
    using TimerHandlers = std::vector<Steinberg::IPtr<TimerHandler>>;

    EventHandlers eventHandlers;
    TimerHandlers timerHandlers;
    Steinberg::FUnknownPtr<Steinberg::Linux::IRunLoop> runLoop;
};

//------------------------------------------------------------------------------
struct RunLoop::Impl::EventHandler final : Steinberg::Linux::IEventHandler, public Steinberg::FObject {
    X11::IEventHandler* handler { nullptr };
    bool alive { false };

    void PLUGIN_API onFDIsSet(Steinberg::Linux::FileDescriptor) override;

    DELEGATE_REFCOUNT(Steinberg::FObject)
    DEFINE_INTERFACES
    DEF_INTERFACE(Steinberg::Linux::IEventHandler)
    END_DEFINE_INTERFACES(Steinberg::FObject)
};

struct RunLoop::Impl::TimerHandler final : Steinberg::Linux::ITimerHandler, public Steinberg::FObject {
    X11::ITimerHandler* handler { nullptr };
    bool alive { false };

    void PLUGIN_API onTimer() override;

    DELEGATE_REFCOUNT(Steinberg::FObject)
    DEFINE_INTERFACES
    DEF_INTERFACE(Steinberg::Linux::ITimerHandler)
    END_DEFINE_INTERFACES(Steinberg::FObject)
};

//------------------------------------------------------------------------------
void PLUGIN_API RunLoop::Impl::EventHandler::onFDIsSet(Steinberg::Linux::FileDescriptor)
{
    SharedPointer<RunLoop> runLoop = RunLoop::get();
    if (!runLoop) {
        fprintf(stderr, "[x11] event has fired without active runloop\n");
        return;
    }

    if (alive && handler)
        handler->onEvent();
}

void PLUGIN_API RunLoop::Impl::TimerHandler::onTimer()
{
    SharedPointer<RunLoop> runLoop = RunLoop::get();
    if (!runLoop) {
        fprintf(stderr, "[x11] timer has fired without active runloop\n");
        return;
    }

    if (alive && handler)
        handler->onTimer();
}

//------------------------------------------------------------------------------
RunLoop::RunLoop(Steinberg::FUnknown* runLoop)
    : impl(new Impl)
{
    impl->runLoop = runLoop;
}

RunLoop::~RunLoop()
{
    //dumpCurrentState();

    if (0) {
        // remove any leftover handlers
        for (size_t i = 0; i < impl->eventHandlers.size(); ++i) {
            const auto& eh = impl->eventHandlers[i];
            if (eh->alive && eh->handler) {
                impl->runLoop->unregisterEventHandler(eh.get());
            }
        }
        for (size_t i = 0; i < impl->timerHandlers.size(); ++i) {
            const auto& th = impl->timerHandlers[i];
            if (th->alive && th->handler) {
                impl->runLoop->unregisterTimer(th.get());
            }
        }
    }
}

SharedPointer<RunLoop> RunLoop::get()
{
    return X11::RunLoop::get().cast<VSTGUI::RunLoop>();
}

void RunLoop::processSomeEvents()
{
    for (size_t i = 0; i < impl->eventHandlers.size(); ++i) {
        const auto& eh = impl->eventHandlers[i];
        if (eh->alive && eh->handler) {
            eh->handler->onEvent();
        }
    }
}

void RunLoop::dumpCurrentState()
{
    fprintf(stderr, "=== X11 runloop ===\n");

    fprintf(stderr, "\t" "Event slots:\n");
    for (size_t i = 0, n = impl->eventHandlers.size(); i < n; ++i) {
        Impl::EventHandler *eh = impl->eventHandlers[i].get();
        fprintf(stderr, "\t\t" "(%lu) alive=%d handler=%p type=%s\n", i, eh->alive, eh->handler, (eh->alive && eh->handler) ? typeid(*eh->handler).name() : "");
    }

    fprintf(stderr, "\t" "Timer slots:\n");
    for (size_t i = 0, n = impl->timerHandlers.size(); i < n; ++i) {
        Impl::TimerHandler *th = impl->timerHandlers[i].get();
        fprintf(stderr, "\t\t" "(%lu) alive=%d handler=%p type=%s\n", i, th->alive, th->handler, (th->alive && th->handler) ? typeid(*th->handler).name() : "");
    }

    fprintf(stderr, "===/X11 runloop ===\n");
}

template <class T>
static void insertHandler(std::vector<Steinberg::IPtr<T>>& list, Steinberg::IPtr<T> handler)
{
    size_t i = 0;
    size_t n = list.size();
    while (i < n && list[i]->alive)
        ++i;
    if (i < n)
        list[i] = handler;
    else
        list.emplace_back(handler);
}

template <class T, class U>
static size_t findHandler(const std::vector<Steinberg::IPtr<T>>& list, U* handler)
{
    for (size_t i = 0, n = list.size(); i < n; ++i) {
        if (list[i]->alive && list[i]->handler == handler)
            return i;
    }
    return ~size_t(0);
}

bool RunLoop::registerEventHandler(int fd, X11::IEventHandler* handler)
{
    if (!impl->runLoop)
        return false;

    auto smtgHandler = Steinberg::owned(new Impl::EventHandler);
    smtgHandler->handler = handler;
    smtgHandler->alive = true;
    if (impl->runLoop->registerEventHandler(smtgHandler, fd) == Steinberg::kResultTrue) {
        insertHandler(impl->eventHandlers, smtgHandler);
        return true;
    }
    return false;
}

bool RunLoop::unregisterEventHandler(X11::IEventHandler* handler)
{
    if (!impl->runLoop)
        return false;

    size_t index = findHandler(impl->eventHandlers, handler);
    if (index == ~size_t(0))
        return false;

    Impl::EventHandler *eh = impl->eventHandlers[index].get();
    if (impl->runLoop->unregisterEventHandler(eh) != Steinberg::kResultTrue)
        return false;

    eh->alive = false;
    return true;
}

bool RunLoop::registerTimer(uint64_t interval, X11::ITimerHandler* handler)
{
    if (!impl->runLoop)
        return false;

    auto smtgHandler = Steinberg::owned(new Impl::TimerHandler);
    smtgHandler->handler = handler;
    smtgHandler->alive = true;
    if (impl->runLoop->registerTimer(smtgHandler, interval) == Steinberg::kResultTrue) {
        insertHandler(impl->timerHandlers, smtgHandler);
        return true;
    }
    return false;
}

bool RunLoop::unregisterTimer(X11::ITimerHandler* handler)
{
    if (!impl->runLoop)
        return false;

    size_t index = findHandler(impl->timerHandlers, handler);
    if (index == ~size_t(0))
        return false;

    Impl::TimerHandler *th = impl->timerHandlers[index].get();
    if (impl->runLoop->unregisterTimer(th) != Steinberg::kResultTrue)
        return false;

    th->alive = false;
    return true;
}

} // namespace VSTGUI

#endif
