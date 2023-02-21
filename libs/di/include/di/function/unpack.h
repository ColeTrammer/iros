#pragma once

#include <di/concepts/conjunction.h>
#include <di/function/invoke.h>
#include <di/meta/integer_sequence.h>
#include <di/types/prelude.h>

namespace di::function {
namespace detail {
    template<typename Ind>
    struct UnpackFunction {};

    template<typename T, T... values>
    struct UnpackFunction<meta::IntegerSequence<T, values...>> {
        template<typename F>
        requires(concepts::Invocable<F&, meta::IntegerSequence<T, values...>>)
        constexpr decltype(auto) operator()(F&& function) const {
            return function::invoke(function, meta::IntegerSequence<T, values...> {});
        }
    };

}

template<typename Seq>
constexpr inline auto unpack = detail::UnpackFunction<Seq> {};
}
