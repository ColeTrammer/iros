#pragma once

#include "di/container/allocator/forward_declaration.h"
#include "di/sync/dumb_spinlock.h"
#include "di/vocab/error/result.h"
#include "di/vocab/error/status_code_forward_declaration.h"
#include "dius/error.h"

#ifndef DIUS_USE_RUNTIME
#include <mutex>
#include <pthread.h>
#endif

namespace di::sync {
class DumbSpinlock;
}

namespace di::platform {
#ifndef DIUS_USE_RUNTIME
using ThreadId = pthread_t;

using DefaultLock = std::mutex;

inline auto get_current_thread_id() -> ThreadId {
    return pthread_self();
}
#else
using ThreadId = i32;

auto get_current_thread_id() -> ThreadId;

using DefaultLock = sync::DumbSpinlock;
#endif

using DefaultAllocator = container::InfallibleAllocator;
using DefaultFallibleAllocator = container::FallibleAllocator;
}
