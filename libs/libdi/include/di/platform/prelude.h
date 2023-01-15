#pragma once

#include <di/platform/custom.h>

namespace di {
using platform::DefaultAllocator;
using platform::DefaultLock;

using platform::get_current_thread_id;
using platform::ThreadId;
}