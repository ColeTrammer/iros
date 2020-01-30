#pragma once

#ifndef __is_libc
#include <new>
#else
#include <stddef.h>

inline void* operator new(size_t, void* p) {
    return p;
}
inline void* operator new[](size_t, void* p) {

    return p;
}

inline void operator delete(void*, void*) {};
inline void operator delete[](void*, void*) {};
#endif /* __is_libc */

extern "C" {
void* malloc(size_t n);
void* realloc(void* p, size_t n);
void free(void* p);
void* calloc(size_t n, size_t sz);
}

namespace std {
typedef decltype(nullptr) nullptr_t;
}

namespace LIIM {

template<typename T> struct Identity { typedef T Type; };

template<typename T> inline constexpr T&& forward(typename Identity<T>::Type& param) {
    return static_cast<T&&>(param);
}

template<typename T> inline T&& move(T&& arg) {
    return static_cast<T&&>(arg);
}

template<typename T> void swap(T& a, T& b) {
    T temp(a);
    a = b;
    b = temp;
}

}

using LIIM::move;