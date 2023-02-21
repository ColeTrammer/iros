#pragma once

#include <di/meta/list/apply.h>

namespace di::meta {
template<concepts::MetaInvocable Fun>
struct Uncurry {
    template<concepts::TypeList List>
    using Invoke = Apply<Fun, List>;
};
}
