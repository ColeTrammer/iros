#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/expected.h>
#include <di/function/pipeable.h>
#include <di/meta/decay.h>
#include <di/meta/unwrap_ref_rvalue.h>
#include <di/vocab/expected/expected_void_error.h>

namespace di::vocab {
namespace detail {
    struct AsFallibleFunction : function::pipeline::EnablePipeline {
        template<concepts::Expected T>
        requires(concepts::ConstructibleFrom<meta::Decay<T>, T>)
        constexpr meta::Decay<T> operator()(T&& value) const {
            return util::forward<T>(value);
        }

        template<typename T, typename R = Expected<meta::UnwrapRefRValue<T>, void>>
        requires(!concepts::Expected<T> && concepts::ConstructibleFrom<R, T>)
        constexpr R operator()(T&& value) const {
            return util::forward<T>(value);
        }
    };
}

constexpr inline auto as_fallible = detail::AsFallibleFunction {};
}
