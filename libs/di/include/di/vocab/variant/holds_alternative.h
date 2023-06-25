#pragma once

#include <di/meta/algorithm.h>
#include <di/vocab/variant/variant_like.h>
#include <di/vocab/variant/variant_types.h>

namespace di::vocab {
namespace detail {
    template<typename T>
    struct HoldsAlternativeFunction {
        template<concepts::VariantLike Var, typename List = meta::VariantTypes<Var>>
        requires(meta::UniqueType<T, List>)
        constexpr bool operator()(Var const& variant) const {
            constexpr auto expected_index = meta::Lookup<T, List>;
            return variant_index(variant) == expected_index;
        }
    };
}

template<typename T>
constexpr inline auto holds_alternative = detail::HoldsAlternativeFunction<T> {};
}

namespace di {
using vocab::holds_alternative;
}
