#pragma once

#include <di/function/curry.h>
#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/util/move.h>

namespace di::function {
namespace proj_ns {
    template<typename P, typename F>
    class ProjImpl : pipeline::EnablePipeline {
    public:
        template<typename Pn, typename Fn>
        constexpr explicit ProjImpl(Pn&& p, Fn&& f) : m_proj(di::forward<Pn>(p)), m_f(di::forward<Fn>(f)) {}

        constexpr ProjImpl(ProjImpl const&) = default;
        constexpr ProjImpl(ProjImpl&&) = default;

        constexpr auto operator=(ProjImpl const&) -> ProjImpl& = delete;
        constexpr auto operator=(ProjImpl&&) -> ProjImpl& = delete;

        template<typename... Args>
        requires(concepts::Invocable<F&, meta::InvokeResult<P&, Args>...>)
        constexpr auto operator()(Args&&... args) & -> decltype(auto) {
            return invoke(m_f, invoke(m_proj, di::forward<Args>(args))...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const&, meta::InvokeResult<P const&, Args>...>)
        constexpr auto operator()(Args&&... args) const& -> decltype(auto) {
            return invoke(m_f, invoke(m_proj, di::forward<Args>(args))...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F &&, meta::InvokeResult<P&, Args>...>)
        constexpr auto operator()(Args&&... args) && -> decltype(auto) {
            return invoke(di::move(m_f), invoke(m_proj, di::forward<Args>(args))...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const &&, meta::InvokeResult<P const&, Args>...>)
        constexpr auto operator()(Args&&... args) const&& -> decltype(auto) {
            return invoke(di::move(m_f), invoke(m_proj, di::forward<Args>(args))...);
        }

    private:
        P m_proj;
        F m_f;
    };

    struct ProjFunction {
        template<concepts::DecayConstructible P, concepts::DecayConstructible F>
        constexpr auto operator()(P&& predicate, F&& function) const {
            return ProjImpl<meta::Decay<P>, meta::Decay<F>> { di::forward<P>(predicate), di::forward<F>(function) };
        }
    };
}

constexpr inline auto proj = di::curry(proj_ns::ProjFunction {}, c_<2zu>);
}

namespace di {
using function::proj;
}
