#pragma once

#include <di/container/allocator/allocator.h>
#include <di/container/allocator/forward_declaration.h>
#include <di/vocab/error/prelude.h>
#include <dius/error.h>

#ifndef DIUS_USE_RUNTIME
#include <pthread.h>
#endif

namespace di::sync {
class DumbSpinlock;
}

namespace di::platform {
#ifndef DIUS_USE_RUNTIME
using ThreadId = pthread_t;

inline ThreadId get_current_thread_id() {
    return pthread_self();
}
#else
using ThreadId = i32;

inline ThreadId get_current_thread_id() {
    return 0;
}
#endif

using DefaultLock = sync::DumbSpinlock;

template<typename T>
using DefaultAllocator = container::Allocator<T>;

template<typename T>
using DefaultFallibleAllocator = container::FallibleAllocator<T>;

template<typename T>
using DefaultFallibleNewResult = vocab::Result<T>;

constexpr dius::PosixCode default_fallible_allocation_error() {
    return dius::PosixError::NotEnoughMemory;
}
}
