#pragma once

#include "di/meta/algorithm.h"
#include "di/meta/core.h"
#include "di/util/as_const.h"
#include "di/util/forward.h"
#include "di/util/get.h"
#include "di/util/move.h"
#include "di/vocab/variant/variant_alternative.h"
#include "di/vocab/variant/variant_index.h"
#include "di/vocab/variant/variant_size.h"
#include "di/vocab/variant/variant_types.h"

namespace di::concepts {
namespace detail {
    template<typename T, typename Indices>
    struct VariantLikeHelper;

    template<typename T, size_t index>
    concept HasVariantAlernative = requires { typename meta::VariantAlternative<T, index>; };

    template<typename T, size_t index>
    concept HasVariantGet = requires(T variant) {
        util::get<index>(variant);
        util::get<index>(util::as_const(variant));
        util::get<index>(util::move(variant));
        util::get<index>(util::move(util::as_const(variant)));
    };

    template<typename T, size_t... indices>
    struct VariantLikeHelper<T, meta::ListV<indices...>> {
        constexpr static bool value = ((HasVariantAlernative<T, indices> && HasVariantGet<T, indices>) && ...);
    };
}

template<typename T>
concept VariantLike = requires {
    typename meta::VariantTypes<T>;
    vocab::variant_size(types::in_place_type<meta::RemoveCVRef<T>>);
} && requires(T const& variant) {
    { vocab::variant_index(variant) } -> SameAs<size_t>;
} && detail::VariantLikeHelper<T, meta::MakeIndexSequence<meta::VariantSize<T>>>::value;
}
