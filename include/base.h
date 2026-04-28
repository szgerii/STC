#pragma once

#ifndef STC_API
    #if defined(_WIN32) || defined(__CYGWIN__)
        #ifdef STC_BUILDING_DLL
            #define STC_API __declspec(dllexport)
        #else
            #define STC_API __declspec(dllimport)
        #endif
    #else
        #if defined(__GNUC__) || defined(__clang__)
            #define STC_API __attribute__((visibility("default"), used))
        #else
            #define STC_API
        #endif
    #endif
#endif

#if defined(_MSC_VER)
    #define STC_FORCE_INLINE __forceinline
#elif defined(__GNUC__) || defined(__clang__)
    #define STC_FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define STC_FORCE_INLINE inline
#endif
