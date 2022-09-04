#pragma once

#include <di/container/view/iota_view.h>
#include <di/util/declval.h>
#include <di/util/forward.h>
#include <di/util/tag_invoke.h>

namespace di::container::view {
namespace detail {
    struct IotaFunction {
        template<typename T>
        constexpr meta::TagInvokeResult<IotaFunction, T> operator()(T&& value) const {
            return util::tag_invoke(*this, util::forward<T>(value));
        }

        template<typename T, typename Bound>
        constexpr meta::TagInvokeResult<IotaFunction, T, Bound> operator()(T&& value, Bound&& bound) const {
            return util::tag_invoke(*this, util::forward<T>(value), util::forward<Bound>(bound));
        }

        template<typename T>
        requires(requires { container::IotaView(util::declval<T>()); })
        constexpr auto operator()(T&& value) const {
            return container::IotaView(util::forward<T>(value));
        }

        template<typename T, typename Bound>
        requires(requires { container::IotaView(util::declval<T>(), util::declval<Bound>()); })
        constexpr auto operator()(T&& value, Bound&& bound) const {
            return container::IotaView(util::forward<T>(value), util::forward<Bound>(bound));
        }
    };
}

constexpr inline auto iota = detail::IotaFunction {};
}
