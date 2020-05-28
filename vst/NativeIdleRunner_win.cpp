// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "NativeIdleRunner.h"
#include <windows.h>
#include <map>
#include <memory>
#include <mutex>

struct NativeIdleRunner::Impl {
    UINT_PTR timerId = 0;
};

///
struct TimerData {
    void(*cbfn)(void*) = nullptr;
    void* cbdata = nullptr;
};

static std::map<UINT_PTR, std::unique_ptr<TimerData>> gTimerData;
static std::mutex gTimerDataMutex;

static void timerProc(HWND, UINT, UINT_PTR timerId, DWORD)
{
    TimerData* data;

    {
        std::lock_guard<std::mutex> lock(gTimerDataMutex);
        auto it = gTimerData.find(timerId);
        data = (it != gTimerData.end()) ? it->second.get() : nullptr;
    }

    if (!data)
        return;

    data->cbfn(data->cbdata);
}

///
NativeIdleRunner::NativeIdleRunner()
    : impl_(new Impl)
{
}

NativeIdleRunner::~NativeIdleRunner()
{
    stop();
}

void NativeIdleRunner::start(double interval, void(*cbfn)(void*), void* cbdata)
{
    stop();

    std::unique_ptr<TimerData> data { new TimerData };
    data->cbfn = cbfn;
    data->cbdata = cbdata;

    int msInterval = static_cast<int>(interval * 1e3);
    UINT_PTR timerId = SetTimer(nullptr, 0, msInterval, &timerProc);

    std::lock_guard<std::mutex> lock(gTimerDataMutex);
    gTimerData[timerId] = std::move(data);
}

void NativeIdleRunner::stop()
{
    UINT_PTR timerId = impl_->timerId;
    if (timerId == 0)
        return;

    KillTimer(nullptr, timerId);

    std::lock_guard<std::mutex> lock(gTimerDataMutex);
    gTimerData.erase(timerId);;
}
