#pragma once

#include <di/function/index_dispatch.h>
#include <di/function/invoke.h>
#include <di/meta/algorithm.h>
#include <di/meta/constexpr.h>
#include <di/platform/compiler.h>
#include <di/vocab/array/array.h>
#include <di/vocab/md/prelude.h>
#include <di/vocab/variant/variant_like.h>
#include <di/vocab/variant/variant_value.h>

namespace di::vocab {
namespace detail {
    template<typename Idx, typename R, typename Vis, typename... Vars>
    struct VisitHelper {};

    template<size_t... indices, typename R, typename Vis, typename... Vars>
    requires(requires {
        {
            function::invoke_r<R>(util::declval<Vis>(), util::get<indices>(util::declval<Vars>())...)
        } -> concepts::ImplicitlyConvertibleTo<R>;
    })
    struct VisitHelper<meta::List<Constexpr<indices>...>, R, Vis, Vars...> {
        constexpr static R call(Vis&& visitor, Vars&&... variants) {
            return function::invoke_r<R>(util::forward<Vis>(visitor),
                                         util::get<indices>(util::forward<Vars>(variants))...);
        }
    };
}

template<typename R, typename Vis, concepts::VariantLike... Vars,
         typename Indices = meta::CartesianProduct<meta::AsList<meta::MakeIndexSequence<meta::VariantSize<Vars>>>...>>
requires(requires {
    []<concepts::TypeList... Idx>(meta::List<Idx...>) {
        return Array { (&detail::VisitHelper<Idx, R, Vis, Vars...>::call)... };
    }(Indices {});
})
constexpr R visit(Vis&& visitor, Vars&&... variants) {
    auto table = []<concepts::TypeList... Idx>(meta::List<Idx...>) {
        return Array { (&detail::VisitHelper<Idx, R, Vis, Vars...>::call)... };
    }(Indices {});

    auto span = MDSpan { table.data(), Extents<size_t, meta::VariantSize<Vars>...> {} };

#ifdef DI_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
    auto f = span(vocab::variant_index(variants)...);
#ifdef DI_GCC
#pragma GCC diagnostic pop
#endif

    return f(util::forward<Vis>(visitor), util::forward<Vars>(variants)...);
}

template<typename Vis, concepts::VariantLike... Vars,
         typename R = decltype(function::invoke(util::declval<Vis>(), util::get<0>(util::declval<Vars>())...))>
constexpr decltype(auto) visit(Vis&& visitor, Vars&&... variants)
requires(requires { visit<R>(util::forward<Vis>(visitor), util::forward<Vars>(variants)...); })
{
    return visit<R>(util::forward<Vis>(visitor), util::forward<Vars>(variants)...);
}
}

namespace di {
using vocab::visit;
}
