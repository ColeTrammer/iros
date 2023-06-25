#pragma once

#include <di/function/curry_back.h>
#include <di/function/invoke.h>
#include <di/meta/util.h>
#include <di/meta/vocab.h>

namespace di::vocab {
namespace detail {
    struct IfErrorFunction {
        template<concepts::Expected T, concepts::InvocableTo<void, meta::Like<T, meta::ExpectedError<T>>> F>
        requires(concepts::DecayConstructible<T>)
        constexpr auto operator()(T&& expected, F&& function) const {
            if (!expected) {
                function::invoke(util::forward<F>(function), util::forward<T>(expected).error());
            }
            return util::forward<T>(expected);
        }

        template<concepts::Expected T, typename F>
        requires(concepts::DecayConstructible<T> && concepts::LanguageVoid<meta::ExpectedError<T>>)
        constexpr auto operator()(T&& expected, F&&) const {
            return util::forward<T>(expected);
        }
    };
}

constexpr inline auto if_error = function::curry_back(detail::IfErrorFunction {}, meta::c_<2zu>);
}

namespace di {
using vocab::if_error;
}
