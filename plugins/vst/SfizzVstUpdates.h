// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstState.h"
#include <base/source/fobject.h>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <mutex>
#include <cstdint>

/**
 * @brief Update which notifies a FIFO queue of one-time updates
 */
class QueuedUpdates : public Steinberg::FObject {
public:
    using List = std::vector<IPtr<FObject>>;

    void enqueue(IPtr<FObject> update);
    List getUpdates(IDependent* dep);

    void addDependent(IDependent* dep) override;
    void removeDependent(IDependent* dep) override;

    OBJ_METHODS(QueuedUpdates, FObject)

private:
    std::mutex mutex_;
    std::map<IDependent*, List> updates_;
};

/**
 * @brief Update which notifies a single OSC message
 */
class OSCUpdate : public Steinberg::FObject {
public:
    OSCUpdate(const uint8* data, uint32 size);
    const uint8* data() const noexcept { return data_.get(); }
    uint32_t size() const noexcept { return size_; }

    OBJ_METHODS(OSCUpdate, FObject)

private:
    std::unique_ptr<uint8[]> data_;
    uint32 size_ = 0;
};

/**
 * @brief Update which notifies one or more note on/off events
 */
class NoteUpdate : public Steinberg::FObject {
public:
    using Item = std::pair<uint32_t, float>;

    NoteUpdate(const Item* items, uint32 count);
    const Item* events() const noexcept { return events_.get(); }
    const uint32_t count() const noexcept { return count_; }

    OBJ_METHODS(NoteUpdate, FObject)

private:
    std::unique_ptr<Item[]> events_;
    uint32_t count_ = 0;
};

/**
 * @brief Update which notifies a change of SFZ file.
 */
class SfzUpdate : public Steinberg::FObject {
public:
    void setPath(std::string newPath)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        path_ = std::move(newPath);
    }

    std::string getPath() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return path_;
    }

    OBJ_METHODS(SfzUpdate, FObject)

private:
    std::string path_;
    mutable std::mutex mutex_;
};

/**
 * @brief Update which notifies a change of SFZ description.
 */
class SfzDescriptionUpdate : public Steinberg::FObject {
public:
    void setDescription(std::string newDescription)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        description_ = std::move(newDescription);
    }

    std::string getDescription() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return description_;
    }

    OBJ_METHODS(SfzDescriptionUpdate, FObject)

private:
    std::string description_;
    mutable std::mutex mutex_;
};

/**
 * @brief Update which notifies a change of scala file.
 */
class ScalaUpdate : public Steinberg::FObject {
public:
    void setPath(std::string newPath)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        path_ = std::move(newPath);
    }

    std::string getPath() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return path_;
    }

    OBJ_METHODS(ScalaUpdate, FObject)

private:
    std::string path_;
    mutable std::mutex mutex_;
};

/**
 * @brief Update which indicates the playing SFZ status.
 */
class PlayStateUpdate : public Steinberg::FObject {
public:
    void setState(SfizzPlayState newState)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = std::move(newState);
    }

    SfizzPlayState getState() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    OBJ_METHODS(PlayStateUpdate, FObject)

private:
    SfizzPlayState state_ {};
    mutable std::mutex mutex_;
};
