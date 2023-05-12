#pragma once

#include <di/execution/query/forwarding_query.h>

namespace di::concepts {
template<typename T>
concept ForwardingQuery = execution::forwarding_query(T {});
}
