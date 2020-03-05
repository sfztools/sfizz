//          Copyright Jean Pierre Cimalando 2018-2020.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#if defined(__APPLE__)
#include <mach/mach.h>
#elif defined(_WIN32)
#include <limits.h>
#include <windows.h>
#else
#include <semaphore.h>
#include <errno.h>
#endif
#include <stdexcept>

class RTSemaphore {
public:
    explicit RTSemaphore(unsigned value = 0);
    ~RTSemaphore();

    RTSemaphore(const RTSemaphore &) = delete;
    RTSemaphore &operator=(const RTSemaphore &) = delete;

    void post();
    void wait();
    bool try_wait();

private:
#if defined(__APPLE__)
    semaphore_t sem_;
#elif defined(_WIN32)
    HANDLE sem_;
#else
    sem_t sem_;
#endif
};

#if defined(__APPLE__)
inline RTSemaphore::RTSemaphore(unsigned value)
{
    if (semaphore_create(mach_task_self(), &sem_, SYNC_POLICY_FIFO, value) != 0)
        throw std::runtime_error("RTSemaphore::RTSemaphore");
}

inline RTSemaphore::~RTSemaphore()
{
    semaphore_destroy(mach_task_self(), sem_);
}

inline void RTSemaphore::post()
{
    if (semaphore_signal(sem_) != KERN_SUCCESS)
        throw std::runtime_error("RTSemaphore::post");
}

inline void RTSemaphore::wait()
{
    do {
        switch (semaphore_wait(sem_)) {
        case KERN_SUCCESS:
            return;
        case KERN_ABORTED:
            break;
        default:
            throw std::runtime_error("RTSemaphore::wait");
        }
    } while (1);
}

inline bool RTSemaphore::try_wait()
{
    do {
        const mach_timespec_t timeout = {0, 0};
        switch (semaphore_timedwait(sem_, timeout)) {
        case KERN_SUCCESS:
            return true;
        case KERN_OPERATION_TIMED_OUT:
            return false;
        case KERN_ABORTED:
            break;
        default:
            throw std::runtime_error("RTSemaphore::try_wait");
        }
    } while (1);
}
#elif defined(_WIN32)
inline RTSemaphore::RTSemaphore(unsigned value)
{
    sem_ = CreateRTSemaphore(nullptr, value, LONG_MAX, nullptr);
    if (!sem_)
        throw std::runtime_error("RTSemaphore::RTSemaphore");
}

inline RTSemaphore::~RTSemaphore()
{
    CloseHandle(sem_);
}

inline void RTSemaphore::post()
{
    if (!ReleaseRTSemaphore(sem_, 1, nullptr))
        throw std::runtime_error("RTSemaphore::post");
}

inline void RTSemaphore::wait()
{
    if (WaitForSingleObject(sem_, INFINITE) != WAIT_OBJECT_0)
        throw std::runtime_error("RTSemaphore::wait");
}

inline bool RTSemaphore::try_wait()
{
    switch (WaitForSingleObject(sem_, 0)) {
    case WAIT_OBJECT_0:
        return true;
    case WAIT_TIMEOUT:
        return false;
    default:
        throw std::runtime_error("RTSemaphore::try_wait");
    }
}
#else
inline RTSemaphore::RTSemaphore(unsigned value)
{
    if (sem_init(&sem_, 0, value) != 0)
        throw std::runtime_error("RTSemaphore::RTSemaphore");
}

inline RTSemaphore::~RTSemaphore()
{
    sem_destroy(&sem_);
}

inline void RTSemaphore::post()
{
    while (sem_post(&sem_) != 0) {
        if (errno != EINTR)
            throw std::runtime_error("RTSemaphore::post");
    }
}

inline void RTSemaphore::wait()
{
    while (sem_wait(&sem_) != 0) {
        if (errno != EINTR)
            throw std::runtime_error("RTSemaphore::wait");
    }
}

inline bool RTSemaphore::try_wait()
{
    do {
        if (sem_trywait(&sem_) == 0)
            return true;
        switch (errno) {
        case EINTR:
            break;
        case EAGAIN:
            return false;
        default:
            throw std::runtime_error("RTSemaphore::try_wait");
        }
    } while (1);
}
#endif
