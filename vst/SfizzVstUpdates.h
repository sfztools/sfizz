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
    OSCUpdate() noexcept = default;
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
 * @brief Update which notifies a change of file path pseudo-parameter
 * The message ID is used to indicate which path it is.
 */
class FilePathUpdate : public Steinberg::FObject {
public:
    explicit FilePathUpdate(int32 type)
        : type_(type)
    {
    }

    int32 getType() const noexcept
    {
        return type_;
    }

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

    OBJ_METHODS(FilePathUpdate, FObject)

private:
    int32 type_ {};
    std::string path_;
    mutable std::mutex mutex_;
};

enum {
    kFilePathUpdateSfz,
    kFilePathUpdateScala,
};

/**
 * @brief Update which indicates the processor status.
 */
class ProcessorStateUpdate : public Steinberg::FObject {
public:
    void setState(SfizzVstState newState)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        state_ = std::move(newState);
    }

    SfizzVstState getState() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    OBJ_METHODS(ProcessorStateUpdate, FObject)

private:
    SfizzVstState state_;
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
    SfizzPlayState state_;
    mutable std::mutex mutex_;
};
