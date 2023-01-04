#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/list/prelude.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/remove_reference.h>
#include <di/types/prelude.h>

namespace di::vocab {
namespace detail {
    struct VariantTypesFunction {
        template<typename Variant>
        requires(concepts::TagInvocable<VariantTypesFunction, InPlaceType<Variant>>)
        constexpr concepts::TypeList auto operator()(InPlaceType<Variant>) const {
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
