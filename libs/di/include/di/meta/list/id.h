#pragma once

#include <di/meta/list/concepts/meta_invocable.h>
#include <di/meta/list/invoke.h>

namespace di::meta {
template<typename T>
struct Id {
    template<typename...>
    using Invoke = T;

    using Type = T;
};
}
