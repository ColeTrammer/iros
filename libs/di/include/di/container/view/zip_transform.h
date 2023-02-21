#pragma once

#include <di/container/view/empty.h>
#include <di/container/view/zip_transform_view.h>
#include <di/function/tag_invoke.h>

namespace di::container::view {
namespace detail {
    struct ZipTransformFunction;

    template<typename F, typename... Cons>
    concept CustomZipTransform = sizeof
    ...(Cons) > 0 && concepts::TagInvocable<ZipTransformFunction, F, Cons...>;

    template<typename F, typename... Cons>
    concept EmptyZipTransform = sizeof
    ...(Cons) == 0 &&
        concepts::Invocable<meta::Decay<F>&>&& concepts::Object<meta::Decay<meta::InvokeResult<meta::Decay<F>&>>>;

    template<typename F, typename... Cons>
    concept ViewZipTransform = sizeof
    ...(Cons) > 0 && requires(F&& function, Cons&&... containers) {
                         ZipTransformView(util::forward<F>(function), util::forward<Cons>(containers)...);
                     };

    struct ZipTransformFunction {
        template<typename F, concepts::ViewableContainer... Cons>
        requires(CustomZipTransform<F, Cons...> || EmptyZipTransform<F, Cons...> || ViewZipTransform<F, Cons...>)
        constexpr concepts::View auto operator()(F&& function, Cons&&... containers) const {
            if constexpr (CustomZipTransform<F, Cons...>) {
                return function::tag_invoke(*this, util::forward<F>(function), util::forward<Cons>(containers)...);
            } else if constexpr (EmptyZipTransform<F, Cons...>) {
                return empty<meta::Decay<meta::InvokeResult<meta::Decay<F>&>>>();
            } else {
                return ZipTransformView { util::forward<F>(function), util::forward<Cons>(containers)... };
            }
        }
    };
}

constexpr inline auto zip_transform = detail::ZipTransformFunction {};
}
