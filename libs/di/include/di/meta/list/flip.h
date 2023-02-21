#pragma once

#include <di/meta/list/concepts/meta_invocable.h>
#include <di/meta/list/invoke.h>

namespace di::meta {
template<concepts::MetaInvocable MetaFn>
struct Flip {
    template<typename T, typename U>
    using Invoke = meta::Invoke<MetaFn, U, T>;
};
}
