#pragma once

#include <di/container/view/concat_view.h>
#include <di/container/view/empty.h>
#include <di/function/tag_invoke.h>

namespace di::container::view {
namespace detail {
    struct ConcatFunction;

    template<typename... Cons>
    concept CustomConcat = (sizeof...(Cons) > 1) && concepts::TagInvocable<ConcatFunction, Cons...>;

    template<typename... Cons>
    concept AllConcat = sizeof
    ...(Cons) == 1;

    template<typename... Cons>
    concept ViewConcat = requires(Cons&&... containers) { ConcatView { util::forward<Cons>(containers)... }; };

    struct ConcatFunction {
        template<concepts::ViewableContainer... Cons>
        requires(CustomConcat<Cons...> || AllConcat<Cons...> || ViewConcat<Cons...>)
        constexpr /* concepts::View */ auto operator()(Cons&&... containers) const {
            if constexpr (CustomConcat<Cons...>) {
                return function::tag_invoke(*this, util::forward<Cons>(containers)...);
            } else if constexpr (AllConcat<Cons...>) {
                return all(util::forward<Cons>(containers)...);
            } else {
                return ConcatView { util::forward<Cons>(containers)... };
            }
        }
    };
}

constexpr inline auto concat = detail::ConcatFunction {};
}