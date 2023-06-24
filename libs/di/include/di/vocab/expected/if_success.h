#pragma once

#include <di/function/curry_back.h>
#include <di/function/invoke.h>
#include <di/meta/util.h>
#include <di/meta/vocab.h>

namespace di::vocab {
namespace detail {
    struct IfSuccessFunction {
        template<concepts::Expected T, concepts::InvocableTo<void, meta::Like<T, meta::ExpectedValue<T>>> F>
        requires(concepts::DecayConstructible<T>)
        constexpr auto operator()(T&& expected, F&& function) const {
            if (expected) {
                function::invoke(util::forward<F>(function), *util::forward<T>(expected));
            }
            return util::forward<T>(expected);
        }

        template<concepts::Expected T, concepts::InvocableTo<void> F>
        requires(concepts::DecayConstructible<T> && concepts::LanguageVoid<meta::ExpectedValue<T>>)
        constexpr auto operator()(T&& expected, F&& function) const {
            if (expected) {
                function::invoke(util::forward<F>(function));
            }
            return util::forward<T>(expected);
        }
    };
}

constexpr inline auto if_success = function::curry_back(detail::IfSuccessFunction {}, meta::c_<2zu>);
}
