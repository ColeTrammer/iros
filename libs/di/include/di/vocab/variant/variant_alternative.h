#pragma once

#include "di/function/tag_invoke.h"
#include "di/meta/constexpr.h"
#include "di/meta/core.h"
#include "di/types/prelude.h"

namespace di::vocab {
namespace detail {
    struct VariantAlternativeFunction {
        template<typename Variant, size_t index>
        requires(concepts::TagInvocable<VariantAlternativeFunction, InPlaceType<Variant>, Constexpr<index>>)
        constexpr auto operator()(InPlaceType<Variant>, Constexpr<index>) const
            -> meta::TagInvokeResult<VariantAlternativeFunction, InPlaceType<Variant>, Constexpr<index>> {
            return function::tag_invoke(*this, in_place_type<Variant>, c_<index>);
        }
    };
}

constexpr inline auto variant_alternative = detail::VariantAlternativeFunction {};
}

namespace di::meta {
template<typename T, size_t index>
using VariantAlternative = decltype(vocab::variant_alternative(in_place_type<meta::RemoveReference<T>>, c_<index>));
}
