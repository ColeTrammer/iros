#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/forward.h>

namespace di::util {
constexpr inline struct SwapFunction {
    template<typename T, typename U>
    requires(concepts::TagInvocable<SwapFunction, T, U>)
    constexpr void operator()(T&& a, U&& b) const {
        di::function::tag_invoke(*this, util::forward<T>(a), util::forward<U>(b));
    }

    template<typename T, typename U>
    requires(!concepts::TagInvocable<SwapFunction, T&, U&> && concepts::Movable<T> && concepts::Movable<U> &&
             concepts::ConstructibleFrom<T, U> && concepts::ConstructibleFrom<U, T>)
    constexpr void operator()(T& a, U& b) const {
        auto temp = auto(util::forward<T>(a));
        a = util::forward<U>(b);
        b = util::forward<T>(temp);
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
