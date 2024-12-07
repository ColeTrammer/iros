#pragma once

#include "di/container/view/cartesian_product_view.h"
#include "di/container/view/empty.h"
#include "di/function/tag_invoke.h"

namespace di::container::view {
namespace detail {
    struct CartesianProductFunction;

    template<typename... Cons>
    concept CustomCartesianProduct = concepts::TagInvocable<CartesianProductFunction, Cons...>;

    template<typename... Cons>
    concept EmptyCartesianProduct = sizeof...(Cons) == 0;

    template<typename... Cons>
    concept ViewCartesianProduct =
        requires(Cons&&... containers) { CartesianProductView { util::forward<Cons>(containers)... }; };

    struct CartesianProductFunction {
        template<concepts::ViewableContainer... Cons>
        requires(CustomCartesianProduct<Cons...> || EmptyCartesianProduct<Cons...> || ViewCartesianProduct<Cons...>)
        constexpr auto operator()(Cons&&... containers) const -> concepts::View auto {
            if constexpr (CustomCartesianProduct<Cons...>) {
                return function::tag_invoke(*this, util::forward<Cons>(containers)...);
            } else if constexpr (EmptyCartesianProduct<Cons...>) {
                return empty<Tuple<>>;
            } else {
                return CartesianProductView { util::forward<Cons>(containers)... };
            }
        }
    };
}

constexpr inline auto cartesian_product = detail::CartesianProductFunction {};
}

namespace di {
using view::cartesian_product;
}
