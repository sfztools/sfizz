// SPDX-License-Identifier: GPL-3.0

#if !defined(__APPLE__) && !defined(_WIN32)
#include "X11RunLoop.h"
#include "vstgui/lib/platform/linux/x11platform.h"
#include "base/source/fobject.h"

namespace VSTGUI {

RunLoop::RunLoop(Steinberg::FUnknown* runLoop)
    : runLoop(runLoop)
{
}

RunLoop::~RunLoop() {}

SharedPointer<RunLoop> RunLoop::get()
{
    return X11::RunLoop::get().cast<VSTGUI::RunLoop>();
}

struct RunLoop::EventHandler final : Steinberg::Linux::IEventHandler, public Steinberg::FObject {
    X11::IEventHandler* handler { nullptr };
    bool alive { false };

    void PLUGIN_API onFDIsSet(Steinberg::Linux::FileDescriptor) override
    {
        if (alive && handler)
            handler->onEvent();
    }

    DELEGATE_REFCOUNT(Steinberg::FObject)
    DEFINE_INTERFACES
    DEF_INTERFACE(Steinberg::Linux::IEventHandler)
    END_DEFINE_INTERFACES(Steinberg::FObject)
};

struct RunLoop::TimerHandler final : Steinberg::Linux::ITimerHandler, public Steinberg::FObject {
    X11::ITimerHandler* handler { nullptr };
    bool alive { false };

    void PLUGIN_API onTimer() override
    {
        if (alive && handler)
            handler->onTimer();
    }

    DELEGATE_REFCOUNT(Steinberg::FObject)
    DEFINE_INTERFACES
    DEF_INTERFACE(Steinberg::Linux::ITimerHandler)
    END_DEFINE_INTERFACES(Steinberg::FObject)
};


void RunLoop::processSomeEvents()
{
    for (size_t i = 0; i < eventHandlers.size(); ++i) {
        const auto& eh = eventHandlers[i];
        if (eh->alive && eh->handler) {
            eh->handler->onEvent();
        }
    }
}

void RunLoop::cleanupDeadHandlers()
{
    for (size_t i = 0; i < eventHandlers.size(); ++i) {
        const auto& eh = eventHandlers[i];
        if (!eh->alive) {
            runLoop->unregisterEventHandler(eh);
            eventHandlers.erase(eventHandlers.begin() + i--);
        }
    }
    for (size_t i = 0; i < timerHandlers.size(); ++i) {
        const auto& th = timerHandlers[i];
        if (!th->alive) {
            runLoop->unregisterTimer(th);
            timerHandlers.erase(timerHandlers.begin() + i--);
        }
    }
}

bool RunLoop::registerEventHandler(int fd, X11::IEventHandler* handler)
{
    if (!runLoop)
        return false;

    auto smtgHandler = Steinberg::owned(new EventHandler());
    smtgHandler->handler = handler;
    smtgHandler->alive = true;
    if (runLoop->registerEventHandler(smtgHandler, fd) == Steinberg::kResultTrue) {
        eventHandlers.push_back(smtgHandler);
        return true;
    }
    return false;
}

bool RunLoop::unregisterEventHandler(X11::IEventHandler* handler)
{
    if (!runLoop)
        return false;

    for (size_t i = 0; i < eventHandlers.size(); ++i) {
        const auto& eh = eventHandlers[i];
        if (eh->alive && eh->handler == handler) {
            eh->alive = false;
            return true;
        }
    }
    return false;
}

bool RunLoop::registerTimer(uint64_t interval, X11::ITimerHandler* handler)
{
    if (!runLoop)
        return false;

    auto smtgHandler = Steinberg::owned(new TimerHandler());
    smtgHandler->handler = handler;
    smtgHandler->alive = true;
    if (runLoop->registerTimer(smtgHandler, interval) == Steinberg::kResultTrue) {
        timerHandlers.push_back(smtgHandler);
        return true;
    }
    return false;
}

bool RunLoop::unregisterTimer(X11::ITimerHandler* handler)
{
    if (!runLoop)
        return false;

    for (size_t i = 0; i < timerHandlers.size(); ++i) {
        const auto& th = timerHandlers[i];
        if (th->alive && th->handler == handler) {
            th->alive = false;
            return true;
        }
    }
    return false;
}

} // namespace VSTGUI

#endif
