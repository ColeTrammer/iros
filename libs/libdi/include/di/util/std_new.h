#pragma once

#include <di/types/size_t.h>
#ifdef DI_USE_STD
#include <new>
#else
inline void* operator new(di::types::size_t, void* p) {
    return p;
}
inline void* operator new[](di::types::size_t, void* p) {
    return p;
}

inline void operator delete(void*, void*) {};
inline void operator delete[](void*, void*) {};
#endif
