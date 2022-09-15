#pragma once

#include <di/function/index_dispatch.h>
#include <di/function/invoke.h>
#include <di/meta/list.h>
#include <di/meta/make_index_sequence.h>
#include <di/vocab/array/array.h>
#include <di/vocab/variant/variant_like.h>
#include <di/vocab/variant/variant_value.h>

namespace di::vocab {
namespace detail {
    template<typename R, typename Vis, typename Var, typename Indices>
    class CheckVisitorHelper;

    template<typename R, typename Vis, typename Var, size_t... indices>
    struct CheckVisitorHelper<R, Vis, Var, meta::IndexSequence<indices...>> {
        constexpr static bool value = concepts::Conjunction<concepts::InvocableTo<Vis, R, meta::VariantValue<Var, indices>>...>;
    };
}

template<typename Res, typename Vis, concepts::VariantLike Var>
requires(detail::CheckVisitorHelper<Res, Vis, Var, meta::MakeIndexSequence<meta::VariantSize<Var>>>::value)
constexpr Res visit(Vis&& visitor, Var&& variant) {
    return function::index_dispatch<Res, meta::VariantSize<Var>>(
        variant_index(variant),
        []<size_t index>(InPlaceIndex<index>, Vis&& visitor, Var&& variant) -> Res {
            return function::invoke(util::forward<Vis>(visitor), util::get<index>(util::forward<Var>(variant)));
        },
        util::forward<Vis>(visitor), util::forward<Var>(variant));
}
}
