#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/remove_reference.h>
#include <di/types/prelude.h>

namespace di::vocab {
namespace detail {
    struct VariantAlternativeFunction {
        template<typename Tuple, size_t index>
        requires(concepts::TagInvocable<VariantAlternativeFunction, InPlaceType<Tuple>, InPlaceIndex<index>>)
        constexpr meta::TagInvokeResult<VariantAlternativeFunction, InPlaceType<Tuple>, InPlaceIndex<index>>
        operator()(InPlaceType<Tuple>, InPlaceIndex<index>) const;
    };
}

constexpr inline auto variant_alternative = detail::VariantAlternativeFunction {};
}

namespace di::meta {
template<typename T, size_t index>
using VariantAlternative = decltype(vocab::variant_alternative(in_place_type<meta::RemoveReference<T>>, in_place_index<index>));
}
