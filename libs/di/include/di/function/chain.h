#pragma once

#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/function/piped.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function {
namespace detail {
    template<typename F, typename G>
    class ChainFunction : public pipeline::EnablePipeline {
    public:
        template<typename Fn, typename Gn>
        constexpr explicit ChainFunction(Fn&& f, Gn&& g) : m_f(util::forward<Fn>(f)), m_g(util::forward<Gn>(g)) {}

        constexpr ChainFunction(ChainFunction const&) = default;
        constexpr ChainFunction(ChainFunction&&) = default;

        constexpr auto operator=(ChainFunction const&) -> ChainFunction& = delete;
        constexpr auto operator=(ChainFunction&&) -> ChainFunction& = delete;

        template<typename... Args>
        requires(concepts::Invocable<F&, Args...> && concepts::Invocable<G&, meta::InvokeResult<F&, Args...>>)
        constexpr auto operator()(Args&&... args) & -> decltype(auto) {
            return function::invoke(m_g, function::invoke(m_f, util::forward<Args>(args)...));
        }

        template<typename... Args>
        requires(concepts::Invocable<F const&, Args...> &&
                 concepts::Invocable<G const&, meta::InvokeResult<F const&, Args...>>)
        constexpr auto operator()(Args&&... args) const& -> decltype(auto) {
            return function::invoke(m_g, function::invoke(m_f, util::forward<Args>(args)...));
        }

        template<typename... Args>
        requires(concepts::Invocable<F &&, Args...> && concepts::Invocable<G &&, meta::InvokeResult<F &&, Args...>>)
        constexpr auto operator()(Args&&... args) && -> decltype(auto) {
            return function::invoke(util::move(m_g), function::invoke(util::move(m_f), util::forward<Args>(args)...));
        }

        template<typename... Args>
        requires(concepts::Invocable<F const &&, Args...> &&
                 concepts::Invocable<G const &&, meta::InvokeResult<F const &&, Args...>>)
        constexpr auto operator()(Args&&... args) const&& -> decltype(auto) {
            return function::invoke(util::move(m_g), function::invoke(util::move(m_f), util::forward<Args>(args)...));
        }

    private:
        F m_f;
        G m_g;
    };
}

template<typename F>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F>)
constexpr auto chain(F&& f) {
    return function::piped(util::forward<F>(f));
}

template<typename F, typename G, typename... Fs>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F> && concepts::ConstructibleFrom<meta::Decay<G>, G> &&
         (concepts::ConstructibleFrom<meta::Decay<Fs>, Fs> && ...))
constexpr auto chain(F&& f, G&& g, Fs&&... rest) {
    return function::chain(
        detail::ChainFunction<meta::Decay<F>, meta::Decay<G>>(util::forward<F>(f), util::forward<G>(g)),
        util::forward<Fs>(rest)...);
}
}

namespace di {
using function::chain;
}
