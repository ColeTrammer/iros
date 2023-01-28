#pragma once

#include <di/execution/query/get_stop_token.h>

namespace di::meta {
template<typename T>
using StopTokenOf = meta::RemoveCVRef<decltype(execution::get_stop_token(util::declval<T>()))>;
}