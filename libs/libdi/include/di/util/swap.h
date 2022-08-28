#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/movable.h>
#include <di/concepts/move_assignable.h>
#include <di/concepts/move_constructible.h>
#include <di/meta/decay.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/forward.h>
#include <di/util/relocate.h>
#include <di/util/tag_invoke.h>

namespace di::util {
inline constexpr struct SwapFunction {
    template<typename T, typename U>
    requires(concepts::TagInvocable<SwapFunction, T, U>)
    constexpr void operator()(T&& a, U&& b) const {
        di::util::tag_invoke(*this, util::forward<T>(a), util::forward<U>(b));
    }

    template<typename T, typename U>
    requires(!concepts::TagInvocable<SwapFunction, T, U> && concepts::Movable<T> && concepts::Movable<U> &&
             concepts::ConstructibleFrom<T, U> && concepts::ConstructibleFrom<U, T>)
    constexpr void operator()(T&& a, U&& b) const {
        auto temp = auto(util::forward<T>(a));
        a = util::forward<U>(b);
        b = util::forward<T>(temp);
    }

    template<typename T, typename U>
    requires(!concepts::TagInvocable<SwapFunction, T, U> && (!concepts::MoveAssignable<T> || !concepts::MoveAssignable<U>) &&
             concepts::MoveConstructible<T> && concepts::MoveConstructible<U> && concepts::ConstructibleFrom<T, U> &&
             concepts::ConstructibleFrom<U, T>)
    constexpr void operator()(T&& a, U&& b) const {
        if (&a != &b) {
            auto temp = util::relocate(a);
            util::construct_at(&a, util::relocate(b));
            util::construct_at(&b, util::move(temp));
        }
    }
} swap;
}

namespace di::concepts {
template<typename T>
concept Swappable = requires(T& a, T& b) { di::util::swap(a, b); };

template<typename T, typename U>
concept SwappableWith = requires(T&& a, U&& b) {
                            di::util::swap(util::forward<T>(a), util::forward<T>(a));
                            di::util::swap(util::forward<T>(a), util::forward<U>(b));
                            di::util::swap(util::forward<U>(b), util::forward<T>(a));
                            di::util::swap(util::forward<U>(b), util::forward<U>(b));
                        };
}
