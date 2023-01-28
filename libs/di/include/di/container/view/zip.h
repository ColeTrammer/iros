#pragma once

#include <di/container/view/empty.h>
#include <di/container/view/zip_view.h>
#include <di/function/tag_invoke.h>

namespace di::container::view {
namespace detail {
    struct ZipFunction;

    template<typename... Cons>
    concept CustomZip = concepts::TagInvocable<ZipFunction, Cons...>;

    template<typename... Cons>
    concept EmptyZip = sizeof
    ...(Cons) == 0;

    template<typename... Cons>
    concept ViewZip =
        requires(Cons&&... containers) { ZipView<meta::AsView<Cons>...>(util::forward<Cons>(containers)...); };

    struct ZipFunction {
        template<concepts::ViewableContainer... Cons>
        requires(CustomZip<Cons...> || EmptyZip<Cons...> || ViewZip<Cons...>)
        constexpr concepts::View auto operator()(Cons&&... containers) const {
            if constexpr (CustomZip<Cons...>) {
                return function::tag_invoke(*this, util::forward<Cons>(containers)...);
            } else if constexpr (EmptyZip<Cons...>) {
                return empty<Tuple<>>();
            } else {
                return ZipView<meta::AsView<Cons>...>(util::forward<Cons>(containers)...);
            }
        }
    };
}

constexpr inline auto zip = detail::ZipFunction {};
}