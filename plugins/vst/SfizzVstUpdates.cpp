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
OSCUpdate::OSCUpdate(const uint8* data, uint32 size)
{
    uint8* copy = new uint8[size];
    std::copy_n(data, size, copy);
    data_.reset(copy);
    size_ = size;
}

///
NoteUpdate::NoteUpdate(const Item* items, uint32 count)
{
    Item* copy = new Item[count];
    std::copy_n(items, count, copy);
    events_.reset(copy);
    count_ = count;
}
