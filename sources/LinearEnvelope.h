#pragma once
#include "Globals.h"
#include <type_traits>
#include <functional>
#include <absl/types/span.h>

namespace sfz
{

template<class Type>
class LinearEnvelope
{
public:
    LinearEnvelope();
    LinearEnvelope(int maxCapacity, std::function<Type(Type)> function);
    void setMaxCapacity(int maxCapacity);
    void setFunction(std::function<Type(Type)> function);
    void registerEvent(int timestamp, Type inputValue);
    void clear();
    void reset(Type value=0.0);
    void getBlock(absl::Span<Type> output);
private:
    std::function<Type(Type)> function { [](Type input) { return input; } };
    static_assert(std::is_arithmetic<Type>::value);
    std::vector<std::pair<int, Type>> events;
    int maxCapacity { config::defaultSamplesPerBlock };
    Type currentValue { 0.0 };
};

}