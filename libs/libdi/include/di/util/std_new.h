#pragma once

#include <di/util/types.h>
#ifdef DI_USE_STD
#include <new>
#else
inline void* operator new(di::util::size_t, void* p) {
    return p;
}
inline void* operator new[](di::util::size_t, void* p) {
    return p;
}

inline void operator delete(void*, void*) {};
inline void operator delete[](void*, void*) {};
#endif
