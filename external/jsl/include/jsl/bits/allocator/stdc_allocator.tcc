#include "../../allocator"
#include <stdlib.h>

namespace jsl {

inline void *stdc_allocator_traits::allocate(std::size_t n)
{
    return ::malloc(n);
}

inline void stdc_allocator_traits::deallocate(void *p, std::size_t)
{
    ::free(p);
}

}  // namespace jsl
