#pragma once

#include <di/function/pipeable.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/meta/vocab.h>
#include <di/vocab/expected/expected_void_error.h>

namespace di::vocab {
namespace detail {
    struct AsFallibleFunction : function::pipeline::EnablePipeline {
        template<concepts::Expected T>
        requires(concepts::ConstructibleFrom<meta::Decay<T>, T>)
        constexpr auto operator()(T&& value) const -> meta::Decay<T> {
            return util::forward<T>(value);
        }

        template<typename T, typename R = Expected<meta::UnwrapRefRValue<T>, void>>
        requires(!concepts::Expected<T> && concepts::ConstructibleFrom<R, T>)
        constexpr auto operator()(T&& value) const -> R {
            return util::forward<T>(value);
        }
    };
}

constexpr inline auto as_fallible = detail::AsFallibleFunction {};
}

namespace di {
using vocab::as_fallible;
}
