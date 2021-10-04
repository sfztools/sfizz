// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

/*
  This runloop connects to X11 VSTGUI, it connects VST3 and VSTGUI together.
  The Windows and macOS runloops do not need this, the OS-provided
  functionality is used instead.

  Previously, this was based on VSTGUI code provided by Steinberg.
  This is replaced with a rewrite, because the original code has too many
  issues. For example, it has no robustness in case handlers get added or
  removed within the execution of the handler.

  This version allows to call event processing externally, in case the host
  has a defective X11 event loop notifier. (some versions of Bitwig do)
*/

#pragma once
#if !defined(__APPLE__) && !defined(_WIN32)
#include "vstgui/lib/platform/linux/x11frame.h"
#include "pluginterfaces/gui/iplugview.h"
#include <memory>

namespace VSTGUI {

class RunLoop final : public X11::IRunLoop, public AtomicReferenceCounted {
public:
    explicit RunLoop(Steinberg::FUnknown* runLoop);
    ~RunLoop();

    static SharedPointer<RunLoop> get();

    void processSomeEvents();
    void dumpCurrentState();

    // X11::IRunLoop
    bool registerEventHandler(int fd, X11::IEventHandler* handler) override;
    bool unregisterEventHandler(X11::IEventHandler* handler) override;
    bool registerTimer(uint64_t interval, X11::ITimerHandler* handler) override;
    bool unregisterTimer(X11::ITimerHandler* handler) override;

    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace VSTGUI

#endif
