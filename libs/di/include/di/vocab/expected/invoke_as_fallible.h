#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/decay_constructible.h>
#include <di/concepts/expected.h>
#include <di/concepts/language_void.h>
#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/meta/decay.h>
#include <di/meta/unwrap_ref_rvalue.h>
#include <di/vocab/expected/expected_void_error.h>
#include <di/vocab/expected/expected_void_void.h>

namespace di::vocab {
namespace detail {
    struct InvokeAsFallibleFunction : function::pipeline::EnablePipeline {
        template<typename F, typename... Args, concepts::DecayConstructible R = meta::InvokeResult<F, Args...>>
        requires(concepts::Expected<R>)
        constexpr meta::Decay<R> operator()(F&& function, Args&&... args) const {
            return function::invoke(util::forward<F>(function), util::forward<Args>(args)...);
        }

        template<typename F, typename... Args, typename R = meta::InvokeResult<F, Args...>,
                 typename G = Expected<meta::UnwrapRefRValue<R>, void>>
        requires(!concepts::Expected<R> && concepts::ConstructibleFrom<G, R>)
        constexpr G operator()(F&& function, Args&&... args) const {
            return G { function::invoke(util::forward<F>(function), util::forward<Args>(args)...) };
        }

        template<typename F, typename... Args, typename R = meta::InvokeResult<F, Args...>>
        requires(concepts::LanguageVoid<R>)
        constexpr Expected<void, void> operator()(F&& function, Args&&... args) const {
            function::invoke(util::forward<F>(function), util::forward<Args>(args)...);
            return {};
        }
    };
}

constexpr inline auto invoke_as_fallible = detail::InvokeAsFallibleFunction {};
}
