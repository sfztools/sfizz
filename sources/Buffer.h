#pragma once
#include "Globals.h"
#include <type_traits>
#include <cstdlib>
#include <memory>

template<class Type, unsigned int Alignment = config::defaultAlignment>
class Buffer
{
static_assert(std::is_arithmetic<Type>::value, "Type should be arithmetic");
static_assert(Alignment == 0 || Alignment == 4 || Alignment == 8 || Alignment == 16, "Bad alignment value");
static constexpr auto AlignmentMask { Alignment - 1 };
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
        auto* newData = largerData != nullptr ? std::realloc(largerData, tempSize * sizeof(Type)) : std::malloc(tempSize * sizeof(Type));
        if (newData == nullptr)
            return false;

        largerSize = tempSize;
        alignedSize = newSize;
        largerData = static_cast<Type*>(newData);
        alignedData = static_cast<Type*>(std::align(Alignment, alignedSize, newData, tempSize));
        return true;
    }

    Type* data() { return alignedData; }
    void clear()
    {
        std::free(largerData);
        largerSize = 0;
        alignedSize = 0;
        alignedData = nullptr;
    }
    ~Buffer()
    {
        std::free(largerData);
    }

    Type& operator[](int idx) { return *(alignedData + idx); }
    size_t size() const noexcept { return alignedSize; }
    bool empty() const noexcept { return alignedSize == 0; }
    
    Type* begin() noexcept { return data(); }
    Type* end() noexcept { return data() + alignedSize; }
    Type* alignedEnd() noexcept { return data() + alignedSize; }
    // const Type* cbegin() const noexcept { return data(); }
    // const Type* cend() const noexcept { return data() + alignedSize; }
private:
    size_t largerSize { 0 };
    size_t alignedSize { 0 };
    Type* largerData { nullptr };
    Type* alignedData { nullptr };
};