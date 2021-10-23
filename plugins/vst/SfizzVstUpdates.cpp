// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstUpdates.h"
#include <algorithm>
#include <cstring>

void QueuedUpdates::enqueue(IPtr<FObject> update)
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (std::pair<IDependent* const, List>& item : updates_)
        item.second.push_back(update);
}

auto QueuedUpdates::getUpdates(IDependent* dep) -> List
{
    std::lock_guard<std::mutex> lock(mutex_);
    List list;
    auto it = updates_.find(dep);
    if (it != updates_.end())
        std::swap(list, it->second);
    return list;
}

void QueuedUpdates::addDependent(IDependent* dep)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FObject::addDependent(dep);
    updates_.emplace(dep, List());
}

void QueuedUpdates::removeDependent(IDependent* dep)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FObject::removeDependent(dep);
    updates_.erase(dep);
}

///
bool OSCUpdate::saveToAttributes(Vst::IAttributeList* attrs) const
{
    return attrs->setBinary("Data", data(), size()) == kResultTrue;
}

bool OSCUpdate::loadFromAttributes(Vst::IAttributeList* attrs)
{
    const void* data;
    uint32 size;
    if (attrs->getBinary("Data", data, size) != kResultTrue)
        return false;
    const uint8* data8 = reinterpret_cast<const uint8*>(data);
    data_.assign(data8, data8 + size);
    return true;
}

///
bool NoteUpdate::saveToAttributes(Vst::IAttributeList* attrs) const
{
    return attrs->setBinary("Events", events_.data(), events_.size() * sizeof(Item)) == kResultTrue;
}

bool NoteUpdate::loadFromAttributes(Vst::IAttributeList* attrs)
{
    const void* binData = nullptr;
    uint32 binSize = 0;
    if (attrs->getBinary("Events", binData, binSize) != kResultTrue)
        return false;

    const Item* events = reinterpret_cast<const Item*>(binData);
    uint32 numEvents = binSize / sizeof(Item);

    events_.assign(events, events + numEvents);
    return true;
}

///
bool FilePathUpdate::saveFilePathAttributes_(Vst::IAttributeList* attrs) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return attrs->setBinary("Path", path_.data(), path_.size()) == kResultTrue;
}

bool FilePathUpdate::loadFilePathAttributes_(Vst::IAttributeList* attrs)
{
    const void* binData = nullptr;
    uint32 binSize = 0;
    if (attrs->getBinary("Path", binData, binSize) != kResultTrue)
        return false;
    std::lock_guard<std::mutex> lock(mutex_);
    path_.assign(reinterpret_cast<const char *>(binData), binSize);
    return true;
}

///
bool SfzUpdate::saveToAttributes(Vst::IAttributeList* attrs) const
{
    return saveFilePathAttributes_(attrs);
}

bool SfzUpdate::loadFromAttributes(Vst::IAttributeList* attrs)
{
    return loadFilePathAttributes_(attrs);
}

///
bool ScalaUpdate::saveToAttributes(Vst::IAttributeList* attrs) const
{
    return saveFilePathAttributes_(attrs);
}

bool ScalaUpdate::loadFromAttributes(Vst::IAttributeList* attrs)
{
    return loadFilePathAttributes_(attrs);
}

///
bool SfzDescriptionUpdate::saveToAttributes(Vst::IAttributeList* attrs) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return attrs->setBinary("Blob", description_.data(), description_.size()) == kResultTrue;
}

bool SfzDescriptionUpdate::loadFromAttributes(Vst::IAttributeList* attrs)
{
    const void* binData = nullptr;
    uint32 binSize = 0;
    if (attrs->getBinary("Blob", binData, binSize) != kResultTrue)
        return false;
    std::lock_guard<std::mutex> lock(mutex_);
    description_.assign(reinterpret_cast<const char *>(binData), binSize);
    return true;
}

///
bool PlayStateUpdate::saveToAttributes(Vst::IAttributeList* attrs) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return attrs->setInt("ActiveVoices", state_.activeVoices) == kResultTrue;
}

bool PlayStateUpdate::loadFromAttributes(Vst::IAttributeList* attrs)
{
    int64 activeVoices;
    if (attrs->getInt("ActiveVoices", activeVoices) != kResultTrue)
        return false;
    std::lock_guard<std::mutex> lock(mutex_);
    state_.activeVoices = static_cast<uint32>(activeVoices);
    return true;
}

///
bool AutomationUpdate::saveToAttributes(Vst::IAttributeList* attrs) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return attrs->setBinary("Items", items_.data(), items_.size() * sizeof(Item)) == kResultTrue;
}

bool AutomationUpdate::loadFromAttributes(Vst::IAttributeList* attrs)
{
    const void* binData = nullptr;
    uint32 binSize = 0;
    if (attrs->getBinary("Items", binData, binSize) != kResultTrue)
        return false;

    const Item* events = reinterpret_cast<const Item*>(binData);
    uint32 numEvents = binSize / sizeof(Item);

    std::lock_guard<std::mutex> lock(mutex_);
    items_.assign(events, events + numEvents);
    return true;
}
