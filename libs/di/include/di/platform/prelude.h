#pragma once

#include "di/platform/custom.h"

namespace di {
using platform::BasicError;
using platform::DefaultAllocator;
using platform::DefaultFallibleAllocator;
using platform::DefaultLock;
using platform::GenericDomain;

using platform::get_current_thread_id;
using platform::ThreadId;
}

#include "di/container/allocator/fallible_allocator.h"
#include "di/container/allocator/infallible_allocator.h"
