#pragma once

#include <di/function/tag_invoke.h>
#include <di/meta/core.h>
#include <di/types/prelude.h>

namespace di::vocab {
namespace detail {
    struct VariantSizeFunction {
        template<typename T>
        requires(concepts::TagInvocableTo<VariantSizeFunction, size_t, InPlaceType<T>>)
        constexpr auto operator()(InPlaceType<T>) const -> size_t {
            return function::tag_invoke(*this, in_place_type<T>);
        }
    };
}

constexpr inline auto variant_size = detail::VariantSizeFunction {};
}

namespace di::meta {
template<typename T>
requires(requires { vocab::variant_size(in_place_type<meta::RemoveCVRef<T>>); })
constexpr inline auto VariantSize = vocab::variant_size(in_place_type<meta::RemoveCVRef<T>>);
}
