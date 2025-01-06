#pragma once

#include "di/container/meta/iterator_reference.h"
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
    private:
        template<typename T>
        requires(CustomIterMove<T>)
        constexpr auto impl(T&& value) const -> meta::TagInvokeResult<IteratorMoveFunction, T> {
            return function::tag_invoke(*this, util::forward<T>(value));
        }

        template<typename T>
        requires(!CustomIterMove<T> && RegularIterMove<T>)
        constexpr auto impl(T&& value) const
            -> meta::AddRValueReference<meta::RemoveReference<meta::IteratorReference<T>>> {
            return di::move(*di::forward<T>(value));
        }

        template<typename T>
        requires(!CustomIterMove<T> && !RegularIterMove<T>)
        constexpr auto impl(T&& value) const -> meta::IteratorReference<T> {
            return *di::forward<T>(value);
        }

    public:
        template<typename T>
        requires(CustomIterMove<T> || RegularIterMove<T> || DerefIterMove<T>)
        constexpr auto operator()(T&& value) const -> decltype(this->impl(util::forward<T>(value))) {
            return this->impl(util::forward<T>(value));
        }
    };
}

constexpr inline auto iterator_move = detail::IteratorMoveFunction {};
}
