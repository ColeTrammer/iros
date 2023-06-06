#pragma once

#include <di/concepts/conjunction.h>
#include <di/function/invoke.h>
#include <di/meta/list/list_v.h>
#include <di/types/prelude.h>

namespace di::function {
namespace detail {
    template<typename Ind>
    struct UnpackFunction {};

    template<auto... values>
    struct UnpackFunction<meta::ListV<values...>> {
        template<typename F>
        requires(concepts::Invocable<F&, meta::ListV<values...>>)
        constexpr decltype(auto) operator()(F&& function) const {
            return function::invoke(function, meta::ListV<values...> {});
        }
    };

}

template<typename Seq>
constexpr inline auto unpack = detail::UnpackFunction<Seq> {};
}
