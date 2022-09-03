#pragma once

#include <di/meta/remove_cvref.h>
#include <di/types/in_place_index.h>
#include <di/types/in_place_type.h>
#include <di/util/tag_invoke.h>

namespace di::vocab::tuple {
struct TupleElementFunction {
    template<typename Tuple, types::size_t index>
    requires(concepts::TagInvocable<TupleElementFunction, types::InPlaceType<Tuple>, types::InPlaceIndex<index>>)
    constexpr meta::TagInvokeResult<TupleElementFunction, types::InPlaceType<Tuple>, types::InPlaceIndex<index>>
    operator()(types::InPlaceType<Tuple>, types::InPlaceIndex<index>) const;
};

constexpr inline auto tuple_element = TupleElementFunction {};
}

namespace di::meta {
template<typename T, types::size_t index>
using TupleElement = decltype(vocab::tuple::tuple_element(types::in_place_type<meta::RemoveCVRef<T>>, types::in_place_index<index>));
}
