#pragma once

// From https://stackoverflow.com/questions/48117908/is-the-a-practical-way-to-emulate-go-language-defer-in-c-or-c-destructors
#include<type_traits>
#include<utility>

template<typename F>
struct deferred
{
    std::decay_t<F> f;
    template<typename G>
    deferred(G&& g) : f{std::forward<G>(g)} {}
    ~deferred() { f(); }
};

template<typename G>
deferred(G&&) -> deferred<G>;

#define CAT_(x, y) x##y
#define CAT(x, y) CAT_(x, y)
#define ANONYMOUS_VAR(x) CAT(x, __LINE__)
#define DEFER deferred ANONYMOUS_VAR(defer_variable) = [&]
