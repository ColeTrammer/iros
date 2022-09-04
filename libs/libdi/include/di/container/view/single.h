#pragma once

#include <di/container/view/single_view.h>
#include <di/meta/decay.h>
#include <di/util/forward.h>
#include <di/util/tag_invoke.h>

namespace di::container::view {
namespace detail {
    struct SingleFunction {
        template<typename T>
        constexpr meta::TagInvokeResult<SingleFunction, T> operator()(T&& value) const {
            return util::tag_invoke(*this, util::forward<T>(value));
        }

        template<typename T>
        constexpr auto operator()(T&& value) const {
            return SingleView<meta::Decay<T>>(util::forward<T>(value));
        }
    };
}

constexpr inline auto single = detail::SingleFunction {};
}
