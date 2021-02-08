// SPDX-License-Identifier: GPL-3.0
/*
  This is a modified version the X11 run loop from vst3editor.cpp.

  This version is edited to add more safeguards to protect against host bugs.
  It also permits to call event processing externally in case the host has a
  defective X11 event loop notifier.
*/

#pragma once
#if !defined(__APPLE__) && !defined(_WIN32)
#include "vstgui/lib/platform/linux/x11frame.h"
#include "pluginterfaces/gui/iplugview.h"

namespace VSTGUI {

class RunLoop final : public X11::IRunLoop, public AtomicReferenceCounted {
public:
    explicit RunLoop(Steinberg::FUnknown* runLoop);
    ~RunLoop();

    static SharedPointer<RunLoop> get();

    void processSomeEvents();
    void cleanupDeadHandlers();

    // X11::IRunLoop
    bool registerEventHandler(int fd, X11::IEventHandler* handler);
    bool unregisterEventHandler(X11::IEventHandler* handler);
    bool registerTimer(uint64_t interval, X11::ITimerHandler* handler);
    bool unregisterTimer(X11::ITimerHandler* handler);

private:
    struct EventHandler;
    struct TimerHandler;

private:
    using EventHandlers = std::vector<Steinberg::IPtr<EventHandler>>;
    using TimerHandlers = std::vector<Steinberg::IPtr<TimerHandler>>;
    EventHandlers eventHandlers;
    TimerHandlers timerHandlers;
    Steinberg::FUnknownPtr<Steinberg::Linux::IRunLoop> runLoop;
};

} // namespace VSTGUI

#endif
