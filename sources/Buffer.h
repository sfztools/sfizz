#pragma once
#include <type_traits>
template<class Type>
class Buffer
{
static_assert(std::is_arithmetic<Type>::value, "Type should be arithmetic");
public:
    Buffer()
    {

    }
    ~Buffer()
    {

    }
private:
    size_t realSize;
    size_t alignment;
    Type* data;
};