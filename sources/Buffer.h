#pragma once
#include "Globals.h"
#include <cstdlib>
#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>

template <class Type, unsigned int Alignment = SIMDConfig::defaultAlignment>
class Buffer
{
public:
    using value_type = std::remove_cv_t<Type>;
    using pointer = value_type *;
    using const_pointer = const value_type *;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = pointer;
    using const_iterator = const_pointer;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    Buffer() 
    {
    }
    Buffer(size_t size)
    {
        resize(size);
    }
    bool resize(size_t newSize)
    {
        if (newSize == 0)
        {
            clear();
            return true;
        }

        auto tempSize = newSize + 2 * AlignmentMask; // To ensure that we have leeway at the beginning and at the end
        auto *newData = paddedData != nullptr ? std::realloc(paddedData, tempSize * sizeof(value_type)) : std::malloc(tempSize * sizeof(value_type));
        if (newData == nullptr)
        {
            return false;
        }

        largerSize = tempSize;
        alignedSize = newSize;
        paddedData = static_cast<pointer>(newData);
        normalData = static_cast<pointer>(std::align(Alignment, alignedSize, newData, tempSize));
        normalEnd = normalData + alignedSize;
        if (auto endMisalignment = (alignedSize & TypeAlignmentMask); endMisalignment != 0)
            _alignedEnd = normalEnd + Alignment - endMisalignment;
        else
            _alignedEnd = normalEnd;

        DBG("Buffer resized (" << newSize << ") at: " << this);
        return true;
    }

    void clear()
    {
        largerSize = 0;
        alignedSize = 0;
        std::free(paddedData);
        normalData = nullptr;
        normalEnd = nullptr;
        _alignedEnd = nullptr;
    }
    ~Buffer()
    {
        std::free(paddedData);
    }

    Buffer(const Buffer<Type> &other)
    {
        if (resize(other.size()))
        {
            std::memcpy(this->data(), other.data(), other.size() * sizeof(value_type));
        }
    }

    Buffer(Buffer<Type> &&other)
    {
        largerSize = std::exchange(other.largerSize, 0);
        alignedSize = std::exchange(other.alignedSize, 0);
        paddedData = std::exchange(other.paddedData, nullptr);
        normalData = std::exchange(other.normalData, nullptr);
        normalEnd = std::exchange(other.normalEnd, nullptr);
        _alignedEnd = std::exchange(other._alignedEnd, nullptr);
    }

    Buffer<Type> &operator=(const Buffer<Type> &other)
    {
        if (this != &other)
        {
            if (resize(other.size()))
                std::memcpy(this->data(), other.data(), other.size() * sizeof(value_type));
        }
        return *this;
    }

    Buffer<Type> &operator=(Buffer<Type> &&other)
    {
        if (this != &other)
        {
            std::free(paddedData);
            largerSize = std::exchange(other.largerSize, 0);
            alignedSize = std::exchange(other.alignedSize, 0);
            paddedData = std::exchange(other.paddedData, nullptr);
            normalData = std::exchange(other.normalData, nullptr);
            normalEnd = std::exchange(other.normalEnd, nullptr);
            _alignedEnd = std::exchange(other._alignedEnd, nullptr);
        }
        return *this;
    }

    Type &operator[](int idx) { return *(normalData + idx); }
    constexpr pointer data() const noexcept { return normalData; }
    constexpr size_type size() const noexcept { return alignedSize; }
    constexpr bool empty() const noexcept { return alignedSize == 0; }
    constexpr iterator begin() noexcept { return data(); }
    constexpr iterator end() noexcept { return normalEnd; }
    constexpr pointer alignedEnd() noexcept { return _alignedEnd; }

private:
    static constexpr auto AlignmentMask{Alignment - 1};
    static constexpr auto TypeAlignment{Alignment / sizeof(value_type)};
    static constexpr auto TypeAlignmentMask{TypeAlignment - 1};
    static_assert(std::is_arithmetic<value_type>::value, "Type should be arithmetic");
    static_assert(Alignment == 0 || Alignment == 4 || Alignment == 8 || Alignment == 16, "Bad alignment value");
    static_assert(TypeAlignment * sizeof(value_type) == Alignment, "The alignment does not appear to be divided by the size of the Type");
    size_type largerSize{0};
    size_type alignedSize{0};
    pointer normalData{nullptr};
    pointer paddedData{nullptr};
    pointer normalEnd{nullptr};
    pointer _alignedEnd{nullptr};
};