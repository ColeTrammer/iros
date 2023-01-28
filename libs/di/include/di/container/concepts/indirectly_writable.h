#pragma once

#include <di/container/meta/iterator_reference.h>
#include <di/util/forward.h>

namespace di::concepts {
template<typename Out, typename T>
concept IndirectlyWritable =
    requires(Out&& out, T&& value) {
        *out = util::forward<T>(value);
        *util::forward<Out>(out) = util::forward<T>(value);
        const_cast<meta::IteratorReference<Out> const&&>(*out) = util::forward<T>(value);
        const_cast<meta::IteratorReference<Out> const&&>(*util::forward<Out>(out)) = util::forward<T>(value);
    };
}
