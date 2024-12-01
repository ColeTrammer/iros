#pragma once

#ifdef DI_CUSTOM_PLATFORM
#include DI_CUSTOM_PLATFORM
#elifdef DI_NO_USE_STD
#include <di/container/allocator/forward_declaration.h>
#include <di/platform/default_generic_domain.h>
#include <di/sync/dumb_spinlock.h>

namespace di::platform {
using ThreadId = int;

inline auto get_current_thread_id() -> ThreadId {
    return 0;
}

using DefaultLock = di::sync::DumbSpinlock;
using DefaultAllocator = container::InfallibleAllocator;
using DefaultFallibleAllocator = container::FallibleAllocator;
}
#else
#include <di/container/allocator/forward_declaration.h>
#include <di/platform/default_generic_domain.h>
#include <di/vocab/error/result.h>
#include <di/vocab/expected/expected_forward_declaration.h>
#include <mutex>
#include <thread>

namespace di::platform {
using ThreadId = std::thread::id;

inline auto get_current_thread_id() -> ThreadId {
    return std::this_thread::get_id();
}

using DefaultLock = std::mutex;

using DefaultAllocator = container::InfallibleAllocator;
using DefaultFallibleAllocator = container::FallibleAllocator;
}
#endif
