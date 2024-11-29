#pragma once

#include <di/meta/operations.h>
#include <di/util/clone.h>
#include <di/util/move.h>

namespace di::util {
namespace detail {
    struct MaybeCloneFunction {
        template<typename T>
        requires((concepts::RValueReference<T> && concepts::MoveConstructible<T>) || concepts::Clonable<T>)
        constexpr auto operator()(T&& value) const {
            if constexpr (concepts::RValueReference<T> && concepts::MoveConstructible<T>) {
                return util::forward<T>(value);
            } else {
                return util::clone(value);
            }
        }
    };
}

constexpr inline auto maybe_clone = detail::MaybeCloneFunction {};
}

namespace di {
using util::maybe_clone;
}
