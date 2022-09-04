#pragma once

#include <di/container/view/repeat_view.h>
#include <di/util/declval.h>
#include <di/util/forward.h>
#include <di/util/tag_invoke.h>

namespace di::container::view {
namespace detail {
    struct RepeatFunction {
        template<typename T>
        constexpr meta::TagInvokeResult<RepeatFunction, T> operator()(T&& value) const {
            return util::tag_invoke(*this, util::forward<T>(value));
        }

        template<typename T, typename Bound>
        constexpr meta::TagInvokeResult<RepeatFunction, T, Bound> operator()(T&& value, Bound&& bound) const {
            return util::tag_invoke(*this, util::forward<T>(value), util::forward<Bound>(bound));
        }

        template<typename T>
        requires(requires { container::RepeatView(util::declval<T>()); })
        constexpr auto operator()(T&& value) const {
            return container::RepeatView(util::forward<T>(value));
        }

        template<typename T, typename Bound>
        requires(requires { container::RepeatView(util::declval<T>(), util::declval<Bound>()); })
        constexpr auto operator()(T&& value, Bound&& bound) const {
            return container::RepeatView(util::forward<T>(value), util::forward<Bound>(bound));
        }
    };
}

constexpr inline auto repeat = detail::RepeatFunction {};
}
