#pragma once

#include <di/function/index_dispatch.h>
#include <di/function/invoke.h>
#include <di/meta/list.h>
#include <di/meta/make_index_sequence.h>
#include <di/vocab/array/array.h>
#include <di/vocab/md/prelude.h>
#include <di/vocab/variant/variant_like.h>
#include <di/vocab/variant/variant_value.h>

namespace di::vocab {
namespace detail {
    template<typename Idx, typename R, typename Vis, typename... Vars>
    struct VisitHelper;

    template<size_t... indices, typename R, typename Vis, typename... Vars>
    struct VisitHelper<meta::List<meta::SizeConstant<indices>...>, R, Vis, Vars...> {
        constexpr static R call(Vis&& visitor, Vars&&... variants) {
            return function::invoke(util::forward<Vis>(visitor), util::get<indices>(util::forward<Vars>(variants))...);
        }
    };
}

template<typename R, typename Vis, concepts::VariantLike... Vars>
constexpr R visit(Vis&& visitor, Vars&&... variants) {
    using Indices = meta::CartesianProduct<meta::AsList<meta::MakeIndexSequence<meta::VariantSize<Vars>>>...>;

    constexpr auto table = [&]<concepts::TypeList... Idx>(meta::List<Idx...>) {
        return Array { (&detail::VisitHelper<Idx, R, Vis, Vars...>::call)... };
    }
    (Indices {});

    auto span = MDSpan { table.data(), Extents<size_t, meta::VariantSize<Vars>...> {} };

    auto f = span[variant_index(variants)...];
    return f(util::forward<Vis>(visitor), util::forward<Vars>(variants)...);
}

template<typename Vis, concepts::VariantLike... Vars>
constexpr decltype(auto) visit(Vis&& visitor, Vars&&... variants) {
    using R = decltype(function::invoke(util::forward<Vis>(visitor), util::get<0>(util::forward<Vars>(variants))...));
    return visit<R>(util::forward<Vis>(visitor), util::forward<Vars>(variants)...);
}
}
