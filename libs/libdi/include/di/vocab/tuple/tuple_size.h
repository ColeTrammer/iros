#pragma once

#include <di/meta/remove_cvref.h>
#include <di/types/in_place_type.h>
#include <di/types/size_t.h>
#include <di/util/tag_invoke.h>

namespace di::vocab {
struct TupleSizeFunction {
    template<typename T>
    requires(concepts::TagInvocableTo<TupleSizeFunction, types::size_t, types::InPlaceType<T>>)
    constexpr types::size_t operator()(types::InPlaceType<T>) const {
        return util::tag_invoke(*this, types::in_place_type<T>);
    }
};

constexpr inline auto tuple_size = TupleSizeFunction {};
}

namespace di::meta {
template<typename T>
requires(requires { vocab::tuple_size(types::in_place_type<meta::RemoveCVRef<T>>); })
constexpr inline auto TupleSize = vocab::tuple_size(types::in_place_type<meta::RemoveCVRef<T>>);
}
