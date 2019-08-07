#pragma once
#include "Globals.h"
#include <type_traits>
#include <cstdlib>
#include <memory>

template<class Type, unsigned int Alignment = SIMDConfig::defaultAlignment>
class Buffer
{
public:
    Buffer() { }
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
        auto* newData = paddedData != nullptr ? std::realloc(paddedData, tempSize * sizeof(Type)) : std::malloc(tempSize * sizeof(Type));
        if (newData == nullptr)
            return false;

        largerSize = tempSize;
        alignedSize = newSize;
        paddedData = static_cast<Type*>(newData);
        normalData = static_cast<Type*>(std::align(Alignment, alignedSize, newData, tempSize));
        normalEnd = normalData + alignedSize;
        if (auto endMisalignment = (alignedSize & TypeAlignmentMask); endMisalignment != 0)
            _alignedEnd = normalEnd + Alignment - endMisalignment;
        else
            _alignedEnd = normalEnd;
        return true;
    }

    Type* data() { return normalData; }
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

    Type& operator[](int idx) { return *(normalData + idx); }
    size_t size() const noexcept { return alignedSize; }
    bool empty() const noexcept { return alignedSize == 0; }
    Type* begin() noexcept { return data(); }
    Type* end() noexcept { return normalEnd; }
    Type* alignedEnd() noexcept { return _alignedEnd; }
private:
    static constexpr auto AlignmentMask { Alignment - 1 };
    static constexpr auto TypeAlignment { Alignment / sizeof(Type) };
    static constexpr auto TypeAlignmentMask { TypeAlignment - 1 };
    static_assert(std::is_arithmetic<Type>::value, "Type should be arithmetic");
    static_assert(Alignment == 0 || Alignment == 4 || Alignment == 8 || Alignment == 16, "Bad alignment value");
    static_assert(TypeAlignment * sizeof(Type) == Alignment, "The alignment does not appear to be divided by the size of the Type");
    size_t largerSize { 0 };
    size_t alignedSize { 0 };
    Type* normalData { nullptr };
    Type* paddedData { nullptr };
    Type* normalEnd { nullptr };
    Type* _alignedEnd { nullptr };
};