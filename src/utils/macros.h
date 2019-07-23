#pragma once

#ifdef _MSC_VER
#define INLINE __forceinline
#define likely(x) (x)
#define unlikely(x) (x)
#else
#define INLINE __attribute__((always_inline))
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#define UNUSED(x) (void)(x)