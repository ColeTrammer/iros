#pragma once

#include <di/container/concepts/view.h>
#include <di/container/view/reverse_view.h>
#include <di/container/view/view.h>
#include <di/function/pipeline.h>
#include <di/meta/false_type.h>
#include <di/meta/remove_cv.h>
#include <di/meta/true_type.h>
#include <di/util/declval.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    template<typename T>
    struct IsReverseViewHelper : meta::FalseType {};

    template<concepts::View T>
    struct IsReverseViewHelper<ReverseView<T>> : meta::TrueType {};

    template<typename T>
    concept IsReverseView = IsReverseViewHelper<meta::RemoveCV<T>>::value;

    template<typename T>
    struct IsReverseRawViewHelper : meta::FalseType {};

    template<typename I, bool sized>
    struct IsReverseRawViewHelper<View<ReverseIterator<I>, ReverseIterator<I>, sized>> : meta::TrueType {};

    template<typename T>
    concept IsReverseRawView = IsReverseRawViewHelper<meta::RemoveCV<T>>::value;

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
                    return container::View<Iter, Iter, is_sized>(util::forward<Con>(container).end().base(),
                                                                 util::forward<Con>(container).begin().base(),
                                                                 util::forward<Con>(container).size());
                } else {
                    return container::View<Iter, Iter, is_sized>(util::forward<Con>(container).end().base(),
                                                                 util::forward<Con>(container).begin().base());
                }
            } else {
                return ReverseView { util::forward<Con>(container) };
            }
        }
    };
}

constexpr inline auto reverse = detail::ReverseFunction {};
}
