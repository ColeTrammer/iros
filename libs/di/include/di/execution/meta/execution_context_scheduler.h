#pragma once

#include <di/execution/concepts/execution_context.h>

namespace di::meta {
template<concepts::ExecutionContext Context>
using ExecutionContextScheduler = decltype(util::declval<Context&>().get_scheduler());
}
