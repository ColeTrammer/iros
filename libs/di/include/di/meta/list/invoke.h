#pragma once

#include <di/meta/list/concepts/meta_invocable.h>

namespace di::meta {
template<concepts::MetaInvocable Fun, typename... Args>
using Invoke = typename Fun::template Invoke<Args...>;
}