// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

// Note(jpc) same code as used in Surge LV2, I am the original author

#pragma once
#include <list>
#include <chrono>

#include "editor/utility/vstgui_before.h"
#include "vstgui/lib/vstguibase.h"
#if LINUX
#include "vstgui/lib/platform/platform_x11.h"
#include "vstgui/lib/platform/linux/x11platform.h"
#endif
#include "editor/utility/vstgui_after.h"

#if LINUX
class Lv2IdleRunLoop : public VSTGUI::X11::IRunLoop
{
public:
   void execIdle();

   bool registerEventHandler(int fd, VSTGUI::X11::IEventHandler* handler) override;
   bool unregisterEventHandler(VSTGUI::X11::IEventHandler* handler) override;
   bool registerTimer(uint64_t interval, VSTGUI::X11::ITimerHandler* handler) override;
   bool unregisterTimer(VSTGUI::X11::ITimerHandler* handler) override;

   void forget() override
   {}
   void remember() override
   {}

private:
   struct Event
   {
      int fd;
      VSTGUI::X11::IEventHandler* handler;
      bool alive;
   };
   struct Timer
   {
      std::chrono::microseconds interval;
      std::chrono::microseconds counter;
      bool lastTickValid;
      std::chrono::steady_clock::time_point lastTick;
      VSTGUI::X11::ITimerHandler* handler;
      bool alive;
   };

private:
   template <class T> static void garbageCollectDeadHandlers(std::list<T>& handlers);

private:
   std::list<Event> _events;
   std::list<Timer> _timers;
};
#endif

#if LINUX
namespace VSTGUI
{
class SoHandleInitializer {
public:
    SoHandleInitializer();
    ~SoHandleInitializer();
private:
    SoHandleInitializer(const SoHandleInitializer&) = delete;
    SoHandleInitializer& operator=(const SoHandleInitializer&) = delete;
};
}
#endif

#if MAC
namespace VSTGUI
{
class BundleRefInitializer {
public:
    BundleRefInitializer();
    ~BundleRefInitializer();
private:
    BundleRefInitializer(const BundleRefInitializer&) = delete;
    BundleRefInitializer& operator=(const BundleRefInitializer&) = delete;
};
}
#endif

namespace VSTGUI
{
class VSTGUIInitializer {
public:
    VSTGUIInitializer();
    ~VSTGUIInitializer();
private:
    VSTGUIInitializer(const VSTGUIInitializer&) = delete;
    VSTGUIInitializer& operator=(const VSTGUIInitializer&) = delete;
};
}
