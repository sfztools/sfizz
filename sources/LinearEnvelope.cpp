#include "LinearEnvelope.h"
#include "Helpers.h"
#include "SIMDHelpers.h"
#include <absl/algorithm/container.h>

namespace sfz
{

template<class Type>
LinearEnvelope<Type>::LinearEnvelope()
{
    setMaxCapacity(maxCapacity);
}

template<class Type>
LinearEnvelope<Type>::LinearEnvelope(int maxCapacity, std::function<Type(Type)> function)
{
    setMaxCapacity(maxCapacity);
    setFunction(function);
}

template<class Type>
void LinearEnvelope<Type>::setMaxCapacity(int maxCapacity)
{
    events.reserve(maxCapacity);
    this->maxCapacity = maxCapacity;
}

template<class Type>
void LinearEnvelope<Type>::setFunction(std::function<Type(Type)> function)
{
    this->function = function;
}

template<class Type>
void LinearEnvelope<Type>::registerEvent(int timestamp, Type inputValue)
{
    if (static_cast<int>(events.size()) < maxCapacity)
        events.emplace_back(timestamp, function(inputValue));
}

template<class Type>
void LinearEnvelope<Type>::clear()
{
    events.clear();
}

template<class Type>
void LinearEnvelope<Type>::reset(Type value)
{
    clear();
    currentValue = function(value);
}

template<class Type>
void LinearEnvelope<Type>::getBlock(absl::Span<Type> output)
{
    absl::c_sort(events, [](const auto& lhs, const auto& rhs) {
        return lhs.first < rhs.first;
    });
    int index { 0 };

    for (auto& event: events)
    {
        const auto length = min(event.first, static_cast<int>(output.size())) - index;
        if (length == 0)
        {
            currentValue = event.second;
            continue;
        }
        
        const auto step = (event.second - currentValue) / length;
        currentValue = ::linearRamp<Type>(output.subspan(index, length), currentValue, step);
        index += length;
    }

    if (index < static_cast<int>(output.size()))
        ::fill<Type>(output.subspan(index), currentValue);
    
    clear();
}

}