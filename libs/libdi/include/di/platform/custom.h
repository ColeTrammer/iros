#pragma once

#ifdef DI_CUSTOM_PLATFORM
#include DI_CUSTOM_PLATFORM
#else
#include <di/container/allocator/forward_declaration.h>

#include <mutex>
#include <thread>

namespace di::platform {
using ThreadId = std::thread::id;

inline ThreadId get_current_thread_id() {
    return std::this_thread::get_id();
}

using DefaultLock = std::mutex;

template<typename T>
using DefaultAllocator = container::Allocator<T>;
}
#endif