#pragma once

#include <di/container/allocator/forward_declaration.h>
#include <di/types/integers.h>

namespace di::sync {
class DumbSpinlock;
}

namespace di::platform {
using ThreadId = types::i32;

extern ThreadId get_current_thread_id();

using DefaultLock = sync::DumbSpinlock;

template<typename T>
using DefaultAllocator = container::Allocator<T>;
}