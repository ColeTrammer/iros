#pragma once

#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/meta/vocab.h>
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

namespace di {
using vocab::invoke_as_fallible;
}
