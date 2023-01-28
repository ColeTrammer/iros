#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/remove_reference.h>
#include <di/types/in_place_index.h>
#include <di/types/in_place_type.h>

namespace di::vocab {
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
using TupleElement =
    decltype(vocab::tuple_element(types::in_place_type<meta::RemoveReference<T>>, types::in_place_index<index>))::Type;
}
