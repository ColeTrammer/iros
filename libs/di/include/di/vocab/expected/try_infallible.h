#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/expected.h>
#include <di/concepts/language_void.h>
#include <di/function/pipeable.h>
#include <di/meta/expected_error.h>
#include <di/meta/like.h>
#include <di/vocab/expected/expected_void_error.h>
#include <di/vocab/expected/expected_void_void.h>

namespace di::vocab {
namespace detail {
    struct TryInfallibleFunction : function::pipeline::EnablePipeline {
        template<concepts::Expected T>
        requires(concepts::LanguageVoid<meta::ExpectedError<T>> &&
                 concepts::ConstructibleFrom<meta::ExpectedValue<T>, meta::Like<T, meta::ExpectedValue<T>>>)
        constexpr meta::ExpectedValue<T> operator()(T&& value) const {
            return util::forward<T>(value).value();
        }

        constexpr void operator()(Expected<void, void>) const {}

        template<concepts::Expected T>
        requires(!concepts::LanguageVoid<meta::ExpectedError<T>> && concepts::ConstructibleFrom<meta::Decay<T>, T>)
        constexpr meta::Decay<T> operator()(T&& value) const {
            return util::forward<T>(value);
        }
    };
}

constexpr inline auto try_infallible = detail::TryInfallibleFunction {};
}
