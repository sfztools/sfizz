// SPDX-License-Identifier: BSD-2-Clause

// This code is part of the sfizz library and is licensed under a BSD 2-clause
// license. You should have receive a LICENSE.md file along with the code.
// If not, contact the sfizz maintainers at https://github.com/sfztools/sfizz

#include "SfizzVstUpdates.h"
#include <cstring>

OSCUpdate::~OSCUpdate()
{
    clear();
}

void OSCUpdate::clear()
{
    if (allocated_)
        delete[] reinterpret_cast<const uint8_t*>(data_);
    data_ = nullptr;
    size_ = 0;
    allocated_ = false;
}

void OSCUpdate::setMessage(const void* data, uint32_t size, bool copy)
{
    clear();

    if (copy) {
        uint8_t *buffer = new uint8_t[size];
        std::memcpy(buffer, data, size);
        data = buffer;
    }

    data_ = data;
    size_ = size;
    allocated_ = copy;
}
