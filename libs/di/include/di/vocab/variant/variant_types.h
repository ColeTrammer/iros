#pragma once

#include "di/function/tag_invoke.h"
#include "di/meta/algorithm.h"
#include "di/meta/core.h"
#include "di/types/prelude.h"

namespace di::vocab {
namespace detail {
    struct VariantTypesFunction {
        template<typename Variant>
        requires(concepts::TagInvocable<VariantTypesFunction, InPlaceType<Variant>>)
        constexpr auto operator()(InPlaceType<Variant>) const -> concepts::TypeList auto {
            return function::tag_invoke(*this, in_place_type<Variant>);
        }
    };
}

constexpr inline auto variant_types = detail::VariantTypesFunction {};
}

namespace di::meta {
template<typename T>
using VariantTypes = decltype(vocab::variant_types(in_place_type<meta::RemoveCVRef<T>>));
}
