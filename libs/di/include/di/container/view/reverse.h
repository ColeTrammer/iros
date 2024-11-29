#pragma once

#include <di/container/concepts/view.h>
#include <di/container/view/reverse_view.h>
#include <di/container/view/view.h>
#include <di/function/pipeline.h>
#include <di/meta/core.h>
#include <di/util/declval.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    template<typename T>
    concept IsReverseView = concepts::InstanceOf<meta::RemoveCV<T>, ReverseView>;

    template<typename T>
    constexpr inline bool is_reverse_raw_view_helper = false;

    template<typename I, bool sized>
    constexpr inline bool is_reverse_raw_view_helper<View<ReverseIterator<I>, ReverseIterator<I>, sized>> = true;

    template<typename T>
    concept IsReverseRawView = is_reverse_raw_view_helper<meta::RemoveCV<T>>;

    template<typename T>
    concept CanReverseView = requires(T&& container) { ReverseView { util::forward<T>(container) }; };

    struct ReverseFunction : function::pipeline::EnablePipeline {
        template<concepts::ViewableContainer Con>
        requires(IsReverseView<Con> || IsReverseRawView<Con> || CanReverseView<Con>)
        constexpr auto operator()(Con&& container) const {
            if constexpr (IsReverseView<Con>) {
                return util::forward<Con>(container).base();
            } else if constexpr (IsReverseRawView<Con>) {
                using RevIter = decltype(container.begin());
                using Iter = decltype(util::declval<RevIter>().base());
                constexpr bool is_sized = concepts::SizedContainer<Con>;

                if constexpr (is_sized) {
                    return container::View<Iter, Iter, is_sized>(container.end().base(), container.begin().base(),
                                                                 container.size());
                } else {
                    return container::View<Iter, Iter, is_sized>(container.end().base(), container.begin().base());
                }
            } else {
                return ReverseView { util::forward<Con>(container) };
            }
        }
    };
}

constexpr inline auto reverse = detail::ReverseFunction {};
}

namespace di {
using view::reverse;
}
