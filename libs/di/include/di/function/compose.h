#pragma once

#include <di/concepts/constructible_from.h>
#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/function/piped.h>
#include <di/meta/decay.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function {
namespace detail {
    template<typename F, typename G>
    class ComposeFunction : public pipeline::EnablePipeline {
    public:
        template<typename Fn, typename Gn>
        constexpr explicit ComposeFunction(Fn&& f, Gn&& g) : m_f(util::forward<Fn>(f)), m_g(util::forward<Gn>(g)) {}

        constexpr ComposeFunction(ComposeFunction const&) = default;
        constexpr ComposeFunction(ComposeFunction&&) = default;

        constexpr ComposeFunction& operator=(ComposeFunction const&) = delete;
        constexpr ComposeFunction& operator=(ComposeFunction&&) = delete;

        template<typename... Args>
        requires(concepts::Invocable<G&, Args...> && concepts::Invocable<F&, meta::InvokeResult<G&, Args...>>)
        constexpr decltype(auto) operator()(Args&&... args) & {
            return function::invoke(m_f, function::invoke(m_g, util::forward<Args>(args)...));
        }

        template<typename... Args>
        requires(concepts::Invocable<G const&, Args...> &&
                 concepts::Invocable<F const&, meta::InvokeResult<G const&, Args...>>)
        constexpr decltype(auto) operator()(Args&&... args) const& {
            return function::invoke(m_f, function::invoke(m_g, util::forward<Args>(args)...));
        }

        template<typename... Args>
        requires(concepts::Invocable<G &&, Args...> && concepts::Invocable<F &&, meta::InvokeResult<G &&, Args...>>)
        constexpr decltype(auto) operator()(Args&&... args) && {
            return function::invoke(util::move(m_f), function::invoke(util::move(m_g), util::forward<Args>(args)...));
        }

        template<typename... Args>
        requires(concepts::Invocable<G const &&, Args...> &&
                 concepts::Invocable<F const &&, meta::InvokeResult<G const &&, Args...>>)
        constexpr decltype(auto) operator()(Args&&... args) const&& {
            return function::invoke(util::move(m_f), function::invoke(util::move(m_g), util::forward<Args>(args)...));
        }

    private:
        F m_f;
        G m_g;
    };
}

template<typename F>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F>)
constexpr auto compose(F&& f) {
    return function::piped(util::forward<F>(f));
}

template<typename F, typename G, typename... Fs>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F> && concepts::ConstructibleFrom<meta::Decay<G>, G> &&
         (concepts::ConstructibleFrom<meta::Decay<Fs>, Fs> && ...))
constexpr auto compose(F&& f, G&& g, Fs&&... rest) {
    return function::compose(
        detail::ComposeFunction<meta::Decay<F>, meta::Decay<G>>(util::forward<F>(f), util::forward<G>(g)),
        util::forward<Fs>(rest)...);
}
}
