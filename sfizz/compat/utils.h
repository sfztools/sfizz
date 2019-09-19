#pragma once

#ifdef __cplusplus
    #if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
        #define SFZ_HAVE_CXX17 1
        #define SFZ_INLINE inline
    #else
        #define SFZ_HAVE_CXX17 0
        #define SFZ_INLINE
    #endif
#else
    #error unknown error
#endif
