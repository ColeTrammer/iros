#pragma once

#include <di/container/allocator/forward_declaration.h>
#include <pthread.h>

namespace di::sync {
class DumbSpinlock;
}

namespace di::platform {
using ThreadId = pthread_t;

inline ThreadId get_current_thread_id() {
    return pthread_self();
}

using DefaultLock = sync::DumbSpinlock;

template<typename T>
using DefaultAllocator = container::Allocator<T>;
}