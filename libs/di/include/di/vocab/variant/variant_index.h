#pragma once

#include "di/function/tag_invoke.h"
#include "di/meta/core.h"
#include "di/types/prelude.h"

namespace di::vocab {
namespace detail {
    template<typename T>
    concept MemberVariantIndex = requires(T const& variant) {
        { variant.index() } -> concepts::SameAs<size_t>;
    };

    struct VariantIndexFunction {
        template<typename T>
        requires(concepts::TagInvocableTo<VariantIndexFunction, size_t, T const&> || MemberVariantIndex<T>)
        constexpr auto operator()(T const& variant) const -> size_t {
            if constexpr (concepts::TagInvocableTo<VariantIndexFunction, size_t, T const&>) {
                return function::tag_invoke(*this, variant);
            } else {
                return variant.index();
            }
        }
    };
}

constexpr inline auto variant_index = detail::VariantIndexFunction {};
}
