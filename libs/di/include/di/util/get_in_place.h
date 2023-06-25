#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/constexpr.h>
#include <di/types/in_place_type.h>
#include <di/types/size_t.h>

namespace di::util {
struct GetInPlaceFunction {
    template<typename T, types::size_t index>
    constexpr meta::TagInvokeResult<GetInPlaceFunction, Constexpr<index>, T> operator()(Constexpr<index> place_holder,
                                                                                        T&& tuple) const {
        return function::tag_invoke(*this, place_holder, util::forward<T>(tuple));
    }

    template<typename T, typename Type>
    constexpr meta::TagInvokeResult<GetInPlaceFunction, types::InPlaceType<Type>, T>
    operator()(types::InPlaceType<Type> place_holder, T&& tuple) const {
        return function::tag_invoke(*this, place_holder, util::forward<T>(tuple));
    }
};

constexpr inline auto get_in_place = GetInPlaceFunction {};
}

namespace di {
using util::get_in_place;
}
