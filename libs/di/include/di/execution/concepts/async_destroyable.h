#pragma once

#include <di/execution/interface/async_destroy.h>

namespace di::concepts {
template<typename T>
concept AsyncDestroyable = requires { execution::async_destroy<T>(util::declval<T&>()); };
}
