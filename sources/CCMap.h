#include <map>

namespace sfz
{
template<class ValueType>
class CCMap
{
public:
    CCMap(const ValueType& defaultValue)
    : defaultValue(defaultValue) { }

    const ValueType &getWithDefault(int index) const noexcept
    {
        auto it = container.find(index);
        if (it == end(container))
        {
            return defaultValue;
        }
        else
        {
            return it->second;
        }
    }

    ValueType &operator[](const int &key) noexcept
    {
        if (!contains(key))
            container.emplace(key, defaultValue);
        return container.operator[](key);
    }

    inline bool empty() const { return container.empty(); }
    const ValueType &at(int index) const { return container.at(index); }
    bool contains(int index) const noexcept { return container.find(index) != end(container); }
private:
    const ValueType defaultValue;
    std::map<int, ValueType> container;
};
}