// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

// Note(jpc) same code as used in Surge LV2, I am the original author

#include "vstgui_helpers.h"
#include <lv2/ui/ui.h>
#include <algorithm>
#include <memory>
#include <mutex>
#include <stdexcept>
#if LINUX
#include <dlfcn.h>
#include <poll.h>
#endif
#if WINDOWS
#include <windows.h>
#endif
#if MAC
#include "vstgui/plugin-bindings/getpluginbundle.h"
#endif
#include "vstgui/lib/vstguiinit.h"

#if LINUX
void Lv2IdleRunLoop::execIdle()
{
   std::chrono::steady_clock::time_point tick = std::chrono::steady_clock::now();

   for (Event& ev : _events)
   {
      if (!ev.alive)
         continue;

// TODO LV2: fix me, XCB descriptor polling not working at this point
#if 0
        pollfd pfd = {};
        pfd.fd = ev.fd;
        pfd.events = POLLIN|POLLERR|POLLHUP;
        if (poll(&pfd, 1, 0) > 0)
#endif
      {
         ev.handler->onEvent();
      }
   }

   for (Timer& tm : _timers)
   {
      if (!tm.alive)
         continue;

      if (tm.lastTickValid)
      {
         std::chrono::steady_clock::duration duration = tick - tm.lastTick;
         tm.counter += std::chrono::duration_cast<std::chrono::microseconds>(duration);
         if (tm.counter >= tm.interval)
         {
            tm.handler->onTimer();
            tm.counter = std::min(tm.counter - tm.interval, tm.interval);
         }
      }
      tm.lastTick = tick;
      tm.lastTickValid = true;
   }

   garbageCollectDeadHandlers<Event>(_events);
   garbageCollectDeadHandlers<Timer>(_timers);
}

bool Lv2IdleRunLoop::registerEventHandler(int fd, VSTGUI::X11::IEventHandler* handler)
{
   // fprintf(stderr, "registerEventHandler %d %p\n", fd, handler);

   Event ev;
   ev.fd = fd;
   ev.handler = handler;
   ev.alive = true;
   _events.push_back(ev);

   return true;
}

bool Lv2IdleRunLoop::unregisterEventHandler(VSTGUI::X11::IEventHandler* handler)
{
   // fprintf(stderr, "unregisterEventHandler %p\n", handler);

   auto it = std::find_if(_events.begin(), _events.end(), [handler](const Event& ev) -> bool {
      return ev.handler == handler && ev.alive;
   });

   if (it != _events.end())
      it->alive = false;

   return true;
}

bool Lv2IdleRunLoop::registerTimer(uint64_t interval, VSTGUI::X11::ITimerHandler* handler)
{
   // fprintf(stderr, "registerTimer %lu %p\n", interval, handler);

   Timer tm;
   tm.interval = std::chrono::milliseconds(interval);
   tm.counter = std::chrono::microseconds(0);
   tm.lastTickValid = false;
   tm.handler = handler;
   tm.alive = true;
   _timers.push_back(tm);

   return true;
}

bool Lv2IdleRunLoop::unregisterTimer(VSTGUI::X11::ITimerHandler* handler)
{
   // fprintf(stderr, "unregisterTimer %p\n", handler);

   auto it = std::find_if(_timers.begin(), _timers.end(), [handler](const Timer& tm) -> bool {
      return tm.handler == handler && tm.alive;
   });

   if (it != _timers.end())
      it->alive = false;

   return true;
}

template <class T> void Lv2IdleRunLoop::garbageCollectDeadHandlers(std::list<T>& handlers)
{
   auto pos = handlers.begin();
   auto end = handlers.end();

   while (pos != end)
   {
      auto curPos = pos++;
      if (!curPos->alive)
         handlers.erase(curPos);
   }
}
#endif

///
#if LINUX
namespace VSTGUI
{
void* soHandle = nullptr;

static volatile size_t soHandleCount = 0;
static std::mutex soHandleMutex;

SoHandleInitializer::SoHandleInitializer()
{
    std::lock_guard<std::mutex> lock(soHandleMutex);
    if (soHandleCount++ == 0) {
        Dl_info info;
        if (dladdr((void*)&lv2ui_descriptor, &info))
            soHandle = dlopen(info.dli_fname, RTLD_LAZY);
        if (!soHandle)
            throw std::runtime_error("SoHandleInitializer");
    }
}

SoHandleInitializer::~SoHandleInitializer()
{
    std::lock_guard<std::mutex> lock(soHandleMutex);
    if (--soHandleCount == 0) {
        dlclose(soHandle);
        soHandle = nullptr;
    }
}

} // namespace VSTGUI
#endif

///
#if WINDOWS
void* hInstance = nullptr;

__declspec(dllexport)
BOOL WINAPI DllMain(HINSTANCE dllInstance, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
        hInstance = dllInstance;
    return TRUE;
}
#endif

///
#if MAC
namespace VSTGUI
{
void* gBundleRef = nullptr;

static volatile size_t gBundleRefCount = 0;
static std::mutex gBundleRefMutex;

BundleRefInitializer::BundleRefInitializer()
{
    std::lock_guard<std::mutex> lock(gBundleRefMutex);
    if (gBundleRefCount++ == 0) {
        gBundleRef = GetPluginBundle();
        if (!gBundleRef)
            throw std::runtime_error("BundleRefInitializer");
    }
}

BundleRefInitializer::~BundleRefInitializer()
{
    std::lock_guard<std::mutex> lock(gBundleRefMutex);
    if (--gBundleRefCount == 0) {
        CFRelease((CFBundleRef)gBundleRef);
        gBundleRef = nullptr;
    }
}

} // namespace VSTGUI
#endif

namespace VSTGUI {

static volatile size_t gVstguiInitCount = 0;
static std::mutex gVstguiInitMutex;

VSTGUIInitializer::VSTGUIInitializer()
{
    std::lock_guard<std::mutex> lock(gVstguiInitMutex);
    if (gVstguiInitCount++ == 0) {
#if WINDOWS
        VSTGUI::init((HINSTANCE)hInstance);
#elif MAC
        VSTGUI::init((CFBundleRef)gBundleRef);
#else
        VSTGUI::init(soHandle);
#endif
    }
}

VSTGUIInitializer::~VSTGUIInitializer()
{
    std::lock_guard<std::mutex> lock(gVstguiInitMutex);
    if (--gVstguiInitCount == 0) {
        VSTGUI::exit();
    }
}

} // namespace VSTGUI
