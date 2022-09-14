#pragma once

#include <di/concepts/conjunction.h>
#include <di/meta/index_sequence.h>
#include <di/meta/make_index_sequence.h>
#include <di/meta/remove_cvref.h>
#include <di/util/as_const.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/vocab/variant/variant_alternative.h>
#include <di/vocab/variant/variant_index.h>
#include <di/vocab/variant/variant_size.h>

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
    struct VariantLikeHelper<T, meta::IndexSequence<indices...>> {
        constexpr static bool value = Conjunction<HasVariantAlernative<T, indices>...> && Conjunction<HasVariantGet<T, indices>...>;
    };
}

template<typename T>
concept VariantLike = requires { vocab::variant_size(types::in_place_type<meta::RemoveCVRef<T>>); } &&
                      requires(T const& variant) {
                          { variant_index(variant) } -> SameAs<size_t>;
                      } && detail::VariantLikeHelper<T, meta::MakeIndexSequence<meta::VariantSize<T>>>::value;
}
