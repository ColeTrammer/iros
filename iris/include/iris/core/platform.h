#pragma once

#include "di/container/algorithm/max.h"
#include "di/container/allocator/allocation.h"
#include "di/container/allocator/forward_declaration.h"
#include "di/types/integers.h"
#include "di/util/std_new.h"
#include "iris/core/error.h"
#include "iris/core/spinlock.h"

namespace di::sync {
class DumbSpinlock;
}

namespace di::platform {
using ThreadId = types::i32;

extern auto get_current_thread_id() -> ThreadId;

using DefaultLock = iris::Spinlock;

using DefaultAllocator = container::FallibleAllocator;
using DefaultFallibleAllocator = container::FallibleAllocator;
}
