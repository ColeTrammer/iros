#pragma once

#ifdef DI_CUSTOM_PLATFORM
#include DI_CUSTOM_PLATFORM
#else
#include <di/container/allocator/forward_declaration.h>
#include <di/platform/default_generic_domain.h>
#include <di/vocab/error/generic_domain.h>
#include <di/vocab/error/result.h>
#include <di/vocab/expected/expected_forward_declaration.h>

#include <mutex>
#include <thread>

namespace di::platform {
using ThreadId = std::thread::id;

inline ThreadId get_current_thread_id() {
    return std::this_thread::get_id();
}

using DefaultLock = std::mutex;

using DefaultAllocator = container::InfallibleAllocator;
using DefaultFallibleAllocator = container::FallibleAllocator;
}
#endif

namespace di {
using platform::BasicError;
using platform::DefaultAllocator;
using platform::DefaultFallibleAllocator;
using platform::DefaultLock;
using platform::GenericDomain;

using platform::get_current_thread_id;
using platform::ThreadId;
}

#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
