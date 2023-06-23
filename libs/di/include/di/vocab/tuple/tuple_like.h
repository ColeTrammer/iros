#pragma once

#include <di/concepts/tuple.h>
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
    struct TupleLikeHelper<T, meta::ListV<indices...>> {
        constexpr static bool value = ((HasTupleElement<T, indices> && HasTupleGet<T, indices>) &&...);
    };
}

// NOTE: The TupleSize must be less than 256 in this definition because clang has a maximum limit of 256 when expanding
// fold expressions. This can happen in reasonable scenarios because a fixed-size array is a tuple-like type.
template<typename T>
concept TupleLike =
    concepts::Tuple<T> || (requires {
        vocab::tuple_size(types::in_place_type<meta::RemoveCVRef<T>>);
    } && (meta::TupleSize<T> < 256) && detail::TupleLikeHelper<T, meta::MakeIndexSequence<meta::TupleSize<T>>>::value);
}
