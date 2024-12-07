#pragma once

#include "di/function/tag_invoke.h"
#include "di/meta/core.h"
#include "di/meta/language.h"
#include "di/util/forward.h"
#include "di/util/move.h"

namespace di::container {
namespace detail {
    struct IteratorMoveFunction;

    template<typename T>
    concept CustomIterMove = (concepts::Class<meta::RemoveCVRef<T>> || concepts::Enum<meta::RemoveCVRef<T>>) &&
                             concepts::TagInvocable<IteratorMoveFunction, T>;

    template<typename T>
    concept RegularIterMove = requires(T&& value) {
        { *util::forward<T>(value) } -> concepts::LValueReference;
    };

    template<typename T>
    concept DerefIterMove = requires(T&& value) { *util::forward<T>(value); };

    struct IteratorMoveFunction {
        template<typename T>
        requires(CustomIterMove<T> || RegularIterMove<T> || DerefIterMove<T>)
        constexpr auto operator()(T&& value) const -> decltype(auto) {
            if constexpr (CustomIterMove<T>) {
                return function::tag_invoke(*this, util::forward<T>(value));
            } else if constexpr (RegularIterMove<T>) {
                return util::move(*util::forward<T>(value));
            } else {
                return *util::forward<T>(value);
            }
        }
    };
}

constexpr inline auto iterator_move = detail::IteratorMoveFunction {};
}
