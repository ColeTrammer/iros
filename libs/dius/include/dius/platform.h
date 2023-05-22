#pragma once

#include <di/container/allocator/forward_declaration.h>
#include <di/vocab/error/result.h>
#include <di/vocab/error/status_code_forward_declaration.h>
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

ThreadId get_current_thread_id();
#endif

using DefaultLock = sync::DumbSpinlock;

using DefaultAllocator = container::InfallibleAllocator;
using DefaultFallibleAllocator = container::FallibleAllocator;
}
