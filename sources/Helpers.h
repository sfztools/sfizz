#pragma once
#include <string_view>
#include <signal.h>

inline void trimInPlace(std::string_view& s)
{
    const auto leftPosition = s.find_first_not_of(" \r\t\n\f\v");
    if (leftPosition != s.npos)
    {
        s.remove_prefix(leftPosition);
        const auto rightPosition = s.find_last_not_of(" \r\t\n\f\v");
        s.remove_suffix(s.size() - rightPosition - 1);
    }
    else
    {
        s.remove_suffix(s.size());
    }    
}

inline std::string_view trim(std::string_view s)
{
    const auto leftPosition = s.find_first_not_of(" \r\t\n\f\v");
    if (leftPosition != s.npos)
    {
        s.remove_prefix(leftPosition);
        const auto rightPosition = s.find_last_not_of(" \r\t\n\f\v");
        s.remove_suffix(s.size() - rightPosition - 1);
    }
    else
    {
        s.remove_suffix(s.size());
    }
    return s;
}

inline constexpr unsigned int Fnv1aBasis = 0x811C9DC5;
inline constexpr unsigned int Fnv1aPrime = 0x01000193;
inline constexpr unsigned int hash(const char *s, unsigned int h = Fnv1aBasis)
{
    return !*s ? h : hash(s + 1, static_cast<unsigned int>((h ^ *s) * static_cast<unsigned long long>(Fnv1aPrime)));
}

inline unsigned int hash(std::string_view s, unsigned int h = Fnv1aBasis)
{
    if (s.length() > 0)
        return hash(std::string_view(s.data() + 1, s.length() - 1), static_cast<unsigned int>((h ^ s.front()) * static_cast<unsigned long long>(Fnv1aPrime)));

    return h;
}

#ifndef NDEBUG
#if __linux__ || __unix__
    // These trap into the signal library rather than your own sourcecode
    // #define ASSERTFALSE { ::kill(0, SIGTRAP); }
    // #define ASSERTFALSE { raise(SIGTRAP); }
  #define ASSERTFALSE { __asm__("int3"); }
#elif _WIN32 || _WIN64
    #pragma intrinsic (__debugbreak)
    #define ASSERTFALSE { __debugbreak(); }
#else
  #define ASSERTFALSE { __asm int 3; }
#endif
#define ASSERT(expression)  if (!(expression)) ASSERTFALSE
#include <iostream>
#define DBG(ostream) std::cerr << ostream << '\n'
#else
#define ASSERTFALSE 
#define ASSERT(expression)
#define DBG(ostream)
#endif