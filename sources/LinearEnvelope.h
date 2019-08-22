#include "Globals.h"
#include "Helpers.h"
#include "SIMDHelpers.h"
#include <type_traits>
#include <functional>
#include <absl/algorithm/container.h>

namespace sfz
{

template<class Type>
class LinearEnvelope
{
public:
    LinearEnvelope()
    {
        setMaxCapacity(maxCapacity);
    }
    LinearEnvelope(int maxCapacity, std::function<Type(Type)> function)
    {
        setMaxCapacity(maxCapacity);
        setFunction(function);
    }

    void setMaxCapacity(int maxCapacity)
    {
        events.reserve(maxCapacity);
        this->maxCapacity = maxCapacity;
    }

    void setFunction(std::function<Type(Type)> function)
    {
        this->function = function;
    }

    void registerEvent(int timestamp, Type inputValue)
    {
        if (events.size() < maxCapacity)
            events.emplace_back(timestamp, function(inputValue));
    }

    void clear()
    {
        events.clear();
    }

    void reset(Type value)
    {
        clear();
        currentValue = function(value);
    }

    void getBlock(absl::span<Type> output)
    {
        absl::c_sort(events, [](const Event& lhs, const Event& rhs) {
            return lhs.timestamp < rhs.timestamp;
        });
        int index { 0 };

        for (auto& event: events)
        {
            const auto length = min(event.timestamp, output.size()) - index;
            if (length == 0)
                continue;
            
            const auto step = (function(event.value) - currentValue) / length;
            ::linearRamp<Type>(output.subspan(index, length), currentValue, step);
        }

        if (index < output.size())
            ::fill<Type>(output.subspan(index), currentValue);
        
        clear();
    }

private:
    struct Event
    {
        int timestamp;
        Type value;
    };
    std::function<Type(Type)> function { [](Type input) { return input; } };
    static_assert(std::is_numeric<Type>::value);
    std::vector<Event> events;
    int maxCapacity { config::defaultSamplesPerBlock };
    Type currentValue;
};

}