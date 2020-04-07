#pragma once

// From https://stackoverflow.com/questions/48117908/is-the-a-practical-way-to-emulate-go-language-defer-in-c-or-c-destructors
#include<type_traits>
#include<utility>

template<typename F>
struct deferred
{
    F f;
    deferred(F f) : f(f) {}
    ~deferred() { f(); }
};


template <typename F>
deferred<F> deferred_func(F f) {
	return deferred<F>(f);
}

#define CAT_(x, y) x##y
#define CAT(x, y) CAT_(x, y)
#define ANONYMOUS_VAR(x) CAT(x, __LINE__)
#define DEFER(code) auto ANONYMOUS_VAR(defer_variable) = deferred_func([&] { code ; })
