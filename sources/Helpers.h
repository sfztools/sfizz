#pragma once
#include <random>
#include <signal.h>
#include <string_view>

inline void trimInPlace(std::string_view& s)
{
    const auto leftPosition = s.find_first_not_of(" \r\t\n\f\v");
    if (leftPosition != s.npos) {
        s.remove_prefix(leftPosition);
        const auto rightPosition = s.find_last_not_of(" \r\t\n\f\v");
        s.remove_suffix(s.size() - rightPosition - 1);
    } else {
        s.remove_suffix(s.size());
    }
}

inline std::string_view trim(std::string_view s)
{
    const auto leftPosition = s.find_first_not_of(" \r\t\n\f\v");
    if (leftPosition != s.npos) {
        s.remove_prefix(leftPosition);
        const auto rightPosition = s.find_last_not_of(" \r\t\n\f\v");
        s.remove_suffix(s.size() - rightPosition - 1);
    } else {
        s.remove_suffix(s.size());
    }
    return s;
}

inline constexpr unsigned int Fnv1aBasis = 0x811C9DC5;
inline constexpr unsigned int Fnv1aPrime = 0x01000193;
inline constexpr unsigned int hash(const char* s, unsigned int h = Fnv1aBasis)
{
    return !*s ? h : hash(s + 1, static_cast<unsigned int>((h ^ *s) * static_cast<unsigned long long>(Fnv1aPrime)));
}

inline unsigned int hash(std::string_view s, unsigned int h = Fnv1aBasis)
{
    if (s.length() > 0)
        return hash(std::string_view(s.data() + 1, s.length() - 1), static_cast<unsigned int>((h ^ s.front()) * static_cast<unsigned long long>(Fnv1aPrime)));

    return h;
}

template <class T>
inline constexpr T min(T op1, T op2) { return std::min(op1, op2); }
template <class T>
inline constexpr T min(T op1, T op2, T op3) { return std::min(op1, std::min(op2, op3)); }
template <class T>
inline constexpr T min(T op1, T op2, T op3, T op4) { return std::min(op1, std::min(op2, std::min(op3, op4))); }

#ifndef NDEBUG
#if __linux__ || __unix__
// These trap into the signal library rather than your own sourcecode
// #define ASSERTFALSE { ::kill(0, SIGTRAP); }
// #define ASSERTFALSE { raise(SIGTRAP); }
#define ASSERTFALSE      \
    {                    \
        __asm__("int3"); \
    }
#elif _WIN32 || _WIN64
#pragma intrinsic(__debugbreak)
#define ASSERTFALSE     \
    {                   \
        __debugbreak(); \
    }
#else
#define ASSERTFALSE  \
    {                \
        __asm int 3; \
    }
#endif
#define ASSERT(expression) \
    if (!(expression))     \
    ASSERTFALSE
#include <iostream>
#define DBG(ostream) std::cerr << ostream << '\n'
#else
#define ASSERTFALSE
#define ASSERT(expression)
#define DBG(ostream)
#endif

template <class Type>
inline constexpr Type db2pow(Type in)
{
    return std::pow(static_cast<Type>(10.0), in * static_cast<Type>(0.1));
}

template <class Type>
inline constexpr Type pow2db(Type in)
{
    return static_cast<Type>(10.0) * std::log10(in);
}

template <class Type>
inline constexpr Type db2mag(Type in)
{
    return std::pow(static_cast<Type>(10.0), in * static_cast<Type>(0.05));
}

template <class Type>
inline constexpr Type mag2db(Type in)
{
    return static_cast<Type>(20.0) * std::log10(in);
}

namespace Random {
static inline std::random_device randomDevice;
static inline std::mt19937 randomGenerator { randomDevice() };
} // namespace Random

inline float midiNoteFrequency(const int noteNumber)
{
    return 440.0f * std::pow(2.0f, (noteNumber - 69) / 12.0f);
}

template <class Type>
constexpr Type pi { 3.141592653589793238462643383279502884 };
template <class Type>
constexpr Type twoPi { 2 * pi<Type> };
template <class Type>
constexpr Type piTwo { pi<Type> / 2 };

#include <atomic>
template <class Owner>
class LeakDetector {
public:
    LeakDetector()
    {
        objectCounter.count++;
    }
    LeakDetector(const LeakDetector&)
    {
        objectCounter.count++;
    }
    ~LeakDetector()
    {
        objectCounter.count--;
        if (objectCounter.count.load() < 0) {
            DBG("Deleted a dangling pointer for class " << Owner::getClassName());
            // Deleted a dangling pointer!
            ASSERTFALSE;
        }
    }

private:
    struct ObjectCounter {
        ObjectCounter() = default;
        ~ObjectCounter()
        {
            if (auto residualCount = count.load() > 0) {
                DBG("Leaked " << residualCount << " instance(s) of class " << Owner::getClassName());
                // Leaked ojects
                ASSERTFALSE;
            }
        };
        std::atomic<int> count { 0 };
    };
    static inline ObjectCounter objectCounter;
};

#ifndef NDEBUG
#define LEAK_DETECTOR(Class)                             \
    friend class LeakDetector<Class>;                    \
    static const char* getClassName() { return #Class; } \
    LeakDetector<Class> leakDetector;
#else
#define LEAK_DETECTOR(Class)
#endif