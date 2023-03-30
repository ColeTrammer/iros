#pragma once

#include <di/concepts/assignable_from.h>
#include <di/concepts/constructible_from.h>
#include <di/container/concepts/iterator.h>
#include <di/container/meta/iterator_reference.h>
#include <di/container/meta/iterator_rvalue.h>
#include <di/function/tag_invoke.h>
#include <di/meta/remove_cvref.h>
#include <di/util/forward.h>
#include <di/util/move.h>
#include <di/util/swap.h>

namespace di::container {
namespace detail {
    struct IteratorSwapFunction;

    template<typename T, typename U>
    concept CustomIteratorSwap =
        (concepts::Class<meta::RemoveCVRef<T>> ||
         concepts::Enum<meta::RemoveCVRef<T>>) &&concepts::TagInvocable<IteratorSwapFunction, T, U>;

    template<typename T, typename U>
    concept DerefIteratorSwap = concepts::Iterator<T> && concepts::Iterator<U> &&
                                concepts::SwappableWith<meta::IteratorReference<T>, meta::IteratorReference<U>>;

    template<typename T, typename U>
    concept ExchangeIteratorSwap = concepts::Iterator<T> && concepts::Iterator<U> &&
                                   concepts::ConstructibleFrom<meta::IteratorValue<T>, meta::IteratorRValue<U>> &&
                                   concepts::ConstructibleFrom<meta::IteratorValue<U>, meta::IteratorRValue<T>> &&
                                   concepts::AssignableFrom<meta::IteratorValue<T>&, meta::IteratorRValue<U>> &&
                                   concepts::AssignableFrom<meta::IteratorValue<U>&, meta::IteratorRValue<T>>;

    struct IteratorSwapFunction {
        template<typename T, typename U, typename TT = meta::RemoveCVRef<T>, typename UU = meta::RemoveCVRef<U>>
        requires(CustomIteratorSwap<TT, UU> || DerefIteratorSwap<TT, UU> || CustomIteratorSwap<T, U>)
        constexpr void operator()(T&& a, U&& b) const {
            if constexpr (CustomIteratorSwap<T, U>) {
                (void) function::tag_invoke(*this, util::forward<T>(a), util::forward<U>(b));
            } else if constexpr (DerefIteratorSwap<TT, UU>) {
                util::swap(*a, *b);
            } else {
                auto temp = meta::IteratorValue<TT>(iterator_move(a));
                *a = iterator_move(b);
                *b = util::move(temp);
            }
        }
    };
}

constexpr inline auto iterator_swap = detail::IteratorSwapFunction {};
}
