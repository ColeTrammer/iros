#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/constexpr.h>
#include <di/meta/core.h>
#include <di/types/in_place_type.h>

namespace di::vocab {
struct TupleElementFunction {
    template<typename Tuple, types::size_t index>
    requires(concepts::TagInvocable<TupleElementFunction, types::InPlaceType<Tuple>, Constexpr<index>>)
    constexpr auto operator()(types::InPlaceType<Tuple>, Constexpr<index>) const
        -> meta::TagInvokeResult<TupleElementFunction, types::InPlaceType<Tuple>, Constexpr<index>>;
};

constexpr inline auto tuple_element = TupleElementFunction {};
}

namespace di::meta {
template<typename T, types::size_t index>
using TupleElement = decltype(vocab::tuple_element(types::in_place_type<meta::RemoveReference<T>>, c_<index>))::Type;
}
