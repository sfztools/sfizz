// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "base/source/fobject.h"
#include <memory>
#include <mutex>

/**
 * A weak reference implementation for Steinberg FObject.
 *
 * Implementation
 * ==============
 *
 * This takes over the ordinary addRef() and release() methods.
 * The variable `refCount` is accessed manually, under a shared mutex.
 * There is a unique data block which is shared with all weak pointers, the
 * system will null it atomically when the reference count hits zero.
 *
 * Usage
 * =====
 *
 * class MyObject : public FObject, public Weakable<MyObject> {
 *   [...]
 *   WEAKABLE_REFCOUNT_METHODS(MyObject)
 * };
 *
 * WeakPtr<MyObject> ptr = myObject.getWeakPtr();
 */

template <class T>
class Weakable;

#define WEAKABLE_REFCOUNT_METHODS(T)                                               \
public:                                                                            \
    Steinberg::uint32 PLUGIN_API addRef() SMTG_OVERRIDE { return weakAddRef(); }   \
    Steinberg::uint32 PLUGIN_API release() SMTG_OVERRIDE { return weakRelease(); } \
private:                                                                           \
    friend class Weakable<T>;                                                      \
    friend class WeakPtr<T>;

///
template <class T>
struct WeakPtrSharedData : public std::enable_shared_from_this<WeakPtrSharedData<T>> {
    explicit WeakPtrSharedData(T* self) : self_(self) {}
    std::mutex mutex_;
    T* self_ = nullptr;
};

///
template <class T>
class WeakPtr {
    friend class Weakable<T>;
    using SharedData = WeakPtrSharedData<T>;

public:
    WeakPtr() = default;

    Steinberg::IPtr<T> lock()
    {
        std::shared_ptr<SharedData> data = data_.lock();
        if (!data)
            return nullptr;
        std::lock_guard<std::mutex> lock { data->mutex_ };
        T* self = data->self_;
        if (self)
            ++self->refCount; // manually because we are holding the lock
        return Steinberg::IPtr<T>(self, false);
    }

private:
    explicit WeakPtr(std::weak_ptr<SharedData> data) : data_(data) {}
    std::weak_ptr<SharedData> data_;
};

///
template <class T>
class Weakable {
    using SharedData = WeakPtrSharedData<T>;

public:
    Weakable()
        : weakData_(new SharedData(static_cast<T*>(this)))
    {
    }

    WeakPtr<T> getWeakPtr()
    {
        return WeakPtr<T>(weakData_);
    }

protected:
    Steinberg::uint32 weakAddRef() //override
    {
        T* self = static_cast<T*>(this);
        std::lock_guard<std::mutex> lock { weakData_->mutex_ };
        return ++self->refCount;
    }

    Steinberg::uint32 weakRelease() //override
    {
        T* self = static_cast<T*>(this);
        std::shared_ptr<SharedData> data = weakData_;
        std::unique_lock<std::mutex> lock { data->mutex_ };
        Steinberg::uint32 count = --self->refCount;
        if (count == 0) {
            data->self_ = nullptr;
            weakData_.reset();
            self->refCount = -1000;
            lock.unlock();
            delete self;
            return 0;
        }
        return count;
    }

private:
    std::shared_ptr<SharedData> weakData_;
};
