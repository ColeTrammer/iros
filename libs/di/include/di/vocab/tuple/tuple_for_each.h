#pragma once

#include "di/function/invoke.h"
#include "di/vocab/tuple/apply.h"
#include "di/vocab/tuple/tuple.h"
#include "di/vocab/tuple/tuple_like.h"
#include "di/vocab/tuple/tuple_value.h"

namespace di::vocab {
namespace detail {
    template<typename F, typename Tup, typename Ind>
    struct TupleForEachValid;

    template<size_t... indices, typename F, typename Tup>
    struct TupleForEachValid<F, Tup, meta::ListV<indices...>> {
        constexpr static bool value = (concepts::Invocable<F&, meta::TupleValue<Tup, indices>> && ...);
    };
}

template<typename F, concepts::TupleLike Tup>
requires(detail::TupleForEachValid<F, Tup, meta::MakeIndexSequence<meta::TupleSize<Tup>>>::value)
constexpr void tuple_for_each(F&& function, Tup&& tuple) {
    return apply(
        [&]<typename... Types>(Types&&... values) {
            (void) (function::invoke(function, util::forward<Types>(values)), ...);
        },
        util::forward<Tup>(tuple));
}
}

namespace di {
using vocab::tuple_for_each;
}
