#pragma once

#include <di/concepts/conjunction.h>
#include <di/concepts/tuple.h>
#include <di/meta/index_sequence.h>
#include <di/meta/make_index_sequence.h>
#include <di/meta/remove_cvref.h>
#include <di/util/as_const.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/util/move.h>
#include <di/vocab/tuple/tuple_element.h>
#include <di/vocab/tuple/tuple_size.h>

namespace di::concepts {
namespace detail {
    template<typename T, typename Indices>
    struct TupleLikeHelper;

    template<typename T, types::size_t index>
    concept HasTupleElement = requires { typename meta::TupleElement<T, index>; };

    template<typename T, types::size_t index>
    concept HasTupleGet = requires(T tuple) {
                              util::get<index>(tuple);
                              util::get<index>(util::as_const(tuple));
                              util::get<index>(util::move(tuple));
                              util::get<index>(util::move(util::as_const(tuple)));
                          };

    template<typename T, types::size_t... indices>
    struct TupleLikeHelper<T, meta::IndexSequence<indices...>> {
        constexpr static bool value =
            Conjunction<HasTupleElement<T, indices>...> && Conjunction<HasTupleGet<T, indices>...>;
    };
}

template<typename T>
concept TupleLike =
    concepts::Tuple<T> || requires { vocab::tuple_size(types::in_place_type<meta::RemoveCVRef<T>>); } &&
                              detail::TupleLikeHelper<T, meta::MakeIndexSequence<meta::TupleSize<T>>>::value;
}
