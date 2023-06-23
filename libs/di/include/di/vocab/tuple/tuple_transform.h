#pragma once

#include <di/function/invoke.h>
#include <di/vocab/tuple/apply.h>
#include <di/vocab/tuple/tuple.h>
#include <di/vocab/tuple/tuple_like.h>
#include <di/vocab/tuple/tuple_value.h>

namespace di::vocab {
namespace detail {
    template<typename F, typename Tup, typename Ind>
    struct TupleTransformValid;

    template<size_t... indices, typename F, typename Tup>
    struct TupleTransformValid<F, Tup, meta::ListV<indices...>> {
        constexpr static bool value = (concepts::Invocable<F&, meta::TupleValue<Tup, indices>> && ...);
    };
}

template<typename F, concepts::TupleLike Tup>
requires(detail::TupleTransformValid<F, Tup, meta::MakeIndexSequence<meta::TupleSize<Tup>>>::value)
constexpr auto tuple_transform(F&& function, Tup&& tuple) {
    return apply(
        [&]<typename... Types>(Types&&... values) {
            return Tuple<meta::InvokeResult<F&, Types>...>(function::invoke(function, util::forward<Types>(values))...);
        },
        util::forward<Tup>(tuple));
}
}
