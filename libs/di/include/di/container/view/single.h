#pragma once

#include <di/container/view/single_view.h>
#include <di/function/tag_invoke.h>
#include <di/meta/util.h>
#include <di/util/forward.h>

namespace di::container::view {
namespace detail {
    struct SingleFunction {
        template<typename T>
        constexpr meta::TagInvokeResult<SingleFunction, T> operator()(T&& value) const {
            return function::tag_invoke(*this, util::forward<T>(value));
        }

        template<typename T>
        constexpr auto operator()(T&& value) const {
            return SingleView<meta::Decay<T>>(util::forward<T>(value));
        }
    };
}

constexpr inline auto single = detail::SingleFunction {};
}
