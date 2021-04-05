// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#pragma once
#include "SfizzVstState.h"
#include <base/source/fobject.h>
#include <vector>
#include <string>
#include <mutex>
#include <cstdint>

/**
 * @brief Update which notifies a single OSC message
 * Is is supposed to be used synchronously.
 * (ie. FObject::changed or UpdateHandler::triggerUpdates)
 */
class OSCUpdate : public Steinberg::FObject {
public:
    OSCUpdate() = default;
    ~OSCUpdate();
    void clear();
    void setMessage(const void* data, uint32_t size, bool copy);

    const void* data() const noexcept { return data_; }
    const uint32_t size() const noexcept { return size_; }

    OBJ_METHODS(OSCUpdate, FObject)

private:
    const void* data_ = nullptr;
    uint32_t size_ = 0;
    bool allocated_ = false;

private:
    OSCUpdate(const OSCUpdate&) = delete;
    OSCUpdate& operator=(const OSCUpdate&) = delete;
};

/**
 * @brief Update which notifies one or more note on/off events
 * Is is supposed to be used synchronously.
 * (ie. FObject::changed or UpdateHandler::triggerUpdates)
 */
class NoteUpdate : public Steinberg::FObject {
public:
    NoteUpdate() = default;
    ~NoteUpdate();
    void clear();
    void setEvents(const std::pair<uint32_t, float>* events, uint32_t count, bool copy);

    const std::pair<uint32_t, float>* events() const noexcept { return events_; }
    const uint32_t count() const noexcept { return count_; }

    OBJ_METHODS(NoteUpdate, FObject)

private:
    const std::pair<uint32_t, float>* events_ = nullptr;
    uint32_t count_ = 0;
    bool allocated_ = false;

private:
    NoteUpdate(const NoteUpdate&) = delete;
    NoteUpdate& operator=(const NoteUpdate&) = delete;
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
