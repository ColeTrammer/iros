#pragma once

#include <di/platform/custom.h>

namespace di {
using platform::default_fallible_allocation_error;
using platform::DefaultAllocator;
using platform::DefaultFallibleNewResult;
using platform::DefaultLock;

using platform::get_current_thread_id;
using platform::ThreadId;
}
