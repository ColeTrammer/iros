#pragma once

#include <di/execution/query/get_env.h>
#include <di/util/declval.h>

namespace di::meta {
template<typename T>
using EnvOf = decltype(execution::get_env(util::declval<T>()));
}
