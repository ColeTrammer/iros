#pragma once

#include <di/execution/interface/async_destroy.h>

namespace di::meta {
template<typename T>
using AsyncDestroyResult = decltype(execution::async_destroy<T>(util::declval<T&>()));
}
