// SPDX-License-Identifier: BSD-2-Clause

// Copyright (c) 2019-2020, Paul Ferrand, Andrea Zanellato
// All rights reserved.

// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:

// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.

// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
// ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
// (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
// ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once
#include "Config.h"
#include "LeakDetector.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>
#include <atomic>

#ifdef DEBUG
#include <iostream>
#endif
namespace sfz
{

/**
 * @brief      A buffer counting class that tries to track the memory usage.
 */
class BufferCounter
{
public:
    BufferCounter() = default;
    ~BufferCounter()
    {
#ifndef NDEBUG
        std::cout << "Remaining buffers on destruction: " << numBuffers << '\n';
        std::cout << "Total size: " << bytes << '\n';
#endif
    }

    void newBuffer(int size)
    {
        numBuffers++;
        bytes.fetch_add(size);
    }

    void bufferResized(int oldSize, int newSize)
    {
        bytes.fetch_add(newSize);
        bytes.fetch_sub(oldSize);
    }

    void bufferDeleted(int size)
    {
        numBuffers--;
        bytes.fetch_sub(size);
    }

    void bufferDeleted(size_t size)
    {
        bufferDeleted(static_cast<int>(size));
    }

    void bufferResized(size_t oldSize, size_t newSize)
    {
        bufferResized(static_cast<int>(oldSize), static_cast<int>(newSize));
    }

    void newBuffer(size_t size)
    {
        newBuffer(static_cast<int>(size));
    }

    int getNumBuffers() { return numBuffers; }
    int getTotalBytes() { return bytes; }
private:
    std::atomic<int> numBuffers { 0 };
    std::atomic<int> bytes { 0 };
};



/**
 * @brief      A heap buffer structure that tries to align its beginning and
 *             adds a small offset at the end for alignment too.
 *
 *             Apparently on Linux this effort is mostly useless, and in the end
 *             most of the SIMD operations are coded with alignment checks and
 *             sentinels so this class could probably be much simpler. It does
 *             however wrap realloc which in some cases should be a bit more
 *             efficient than allocating a whole new block.
 *
 * @tparam     Type       The buffer type
 * @tparam     Alignment  the required alignment in bytes (defaults to
 *                        SIMDConfig::defaultAlignment)
 */
template <class Type, unsigned int Alignment = SIMDConfig::defaultAlignment>
class Buffer {
public:
    using value_type = typename std::remove_cv<Type>::type;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = size_t;

    /**
     * @brief Construct a new Buffer object that is empty
     *
     */
    Buffer()
    {
        counter().newBuffer(0);
    }

    /**
     * @brief Construct a new Buffer object with size
     *
     * @param size
     */
    Buffer(size_t size)
    {
        counter().newBuffer(0);
        resize(size);
    }

    /**
     * @brief Resizes the buffer. Given that std::realloc may return either the same pointer
     * or a new one, you need to account for both cases in the code if you are using deep pointers.
     *
     * @param newSize the new buffer size in bytes
     * @return true if allocation succeeded
     * @return false otherwise
     */
    bool resize(size_t newSize)
    {
        if (newSize == 0) {
            clear();
            return true;
        }

        auto tempSize = newSize + 2 * AlignmentMask; // To ensure that we have leeway at the beginning and at the end
        auto* newData = paddedData != nullptr ? std::realloc(paddedData, tempSize * sizeof(value_type)) : std::malloc(tempSize * sizeof(value_type));
        if (newData == nullptr) {
            return false;
        }

        counter().bufferResized(largerSize * sizeof(value_type), tempSize * sizeof(value_type));
        largerSize = tempSize;
        alignedSize = newSize;
        paddedData = static_cast<pointer>(newData);
        normalData = static_cast<pointer>(align(Alignment, alignedSize, newData, tempSize));
        normalEnd = normalData + alignedSize;
		auto endMisalignment = (alignedSize & TypeAlignmentMask);
		if (endMisalignment != 0)
            _alignedEnd = normalEnd + Alignment - endMisalignment;
        else
            _alignedEnd = normalEnd;

        return true;
    }

    /**
     * @brief Clear the buffers and frees the underlying memory
     *
     */
    void clear()
    {
        counter().bufferResized(largerSize * sizeof(value_type), static_cast<size_t>(0));
        largerSize = 0;
        alignedSize = 0;
        std::free(paddedData);
        normalData = nullptr;
        normalEnd = nullptr;
        _alignedEnd = nullptr;
    }
    ~Buffer()
    {
        counter().bufferDeleted(largerSize * sizeof(value_type));
        std::free(paddedData);
    }

    /**
     * @brief Construct a new Buffer object from an existing one
     *
     * @param other
     */
    Buffer(const Buffer<Type>& other)
    {
        counter().newBuffer(0);
        if (resize(other.size())) {
            std::memcpy(this->data(), other.data(), other.size() * sizeof(value_type));
        }
    }

    /**
     * @brief Construct a new Buffer object by moving an existing one
     *
     * @param other
     */
    Buffer(Buffer<Type>&& other) = delete;
    // {
    //     if (this != &other) {
    //         counter().bufferDeleted(largerSize * sizeof(value_type));
    //         std::free(paddedData);
    //         largerSize = std::exchange(other.largerSize, 0);
    //         alignedSize = std::exchange(other.alignedSize, 0);
    //         paddedData = std::exchange(other.paddedData, nullptr);
    //         normalData = std::exchange(other.normalData, nullptr);
    //         normalEnd = std::exchange(other.normalEnd, nullptr);
    //         _alignedEnd = std::exchange(other._alignedEnd, nullptr);
    //     }
    // }

    Buffer<Type>& operator=(const Buffer<Type>& other)
    {
        if (this != &other) {
            if (resize(other.size()))
                std::memcpy(this->data(), other.data(), other.size() * sizeof(value_type));
        }
        return *this;
    }

    Buffer<Type>& operator=(Buffer<Type>&& other) = delete;
    // {
    //     if (this != &other) {
    //         counter().bufferDeleted(largerSize * sizeof(value_type));
    //         std::free(paddedData);
    //         largerSize = std::exchange(other.largerSize, 0);
    //         alignedSize = std::exchange(other.alignedSize, 0);
    //         paddedData = std::exchange(other.paddedData, nullptr);
    //         normalData = std::exchange(other.normalData, nullptr);
    //         normalEnd = std::exchange(other.normalEnd, nullptr);
    //         _alignedEnd = std::exchange(other._alignedEnd, nullptr);
    //     }
    //     return *this;
    // }

    Type& operator[](size_t idx) { return *(normalData + idx); }
    constexpr pointer data() const noexcept { return normalData; }
    constexpr size_type size() const noexcept { return alignedSize; }
    constexpr bool empty() const noexcept { return alignedSize == 0; }
    constexpr iterator begin() const noexcept  { return data(); }
    constexpr iterator end() const noexcept { return normalEnd; }
    constexpr pointer alignedEnd() const noexcept { return _alignedEnd; }


    /**
     * @brief      Return the buffer counter object.
     *
     *             TODO: In C++ 17 all of this can be static inline.
     *
     * @return     The buffer counter on which you can call various methods.
     */
    static BufferCounter& counter()
    {
        static BufferCounter counter;
        return counter;
    }
private:
    static constexpr int AlignmentMask { Alignment - 1 };
    static constexpr int TypeAlignment { Alignment / sizeof(value_type) };
    static constexpr int TypeAlignmentMask { TypeAlignment - 1 };
    static_assert(std::is_trivial<value_type>::value, "Type should be trivial");
    static_assert(Alignment == 0 || Alignment == 4 || Alignment == 8 || Alignment == 16 || Alignment == 32, "Bad alignment value");
    static_assert(TypeAlignment * sizeof(value_type) == Alignment || !std::is_arithmetic<value_type>::value,
                  "The alignment does not appear to be divided by the size of the arithmetic Type");
    void* align(std::size_t alignment, std::size_t size, void *&ptr, std::size_t &space )
    {
        std::uintptr_t pn = reinterpret_cast< std::uintptr_t>( ptr );
        std::uintptr_t aligned = ( pn + alignment - 1 ) & - alignment;
        std::size_t padding = aligned - pn;
        if ( space < size + padding ) return nullptr;
        space -= padding;
        return ptr = reinterpret_cast< void * >( aligned );
    }

    size_type largerSize { 0 };
    size_type alignedSize { 0 };
    pointer normalData { nullptr };
    pointer paddedData { nullptr };
    pointer normalEnd { nullptr };
    pointer _alignedEnd { nullptr };
    LEAK_DETECTOR(Buffer);
};

}
