#pragma once

#include <di/meta/list/concepts/meta_invocable.h>
#include <di/meta/list/invoke.h>

namespace di::meta {
template<concepts::MetaInvocable MetaFn, typename... Bound>
struct BindFront {
    template<typename... Rest>
    using Invoke = meta::Invoke<MetaFn, Bound..., Rest...>;
};
}
