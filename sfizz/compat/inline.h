#pragma once

#ifdef __cplusplus
    #if (__cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)
        #define SFZ_INLINE inline
    #else
        #define SFZ_INLINE
    #endif
#else
    #error unknown error
#endif
