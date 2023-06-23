#pragma once

#include <di/concepts/constructible_from.h>
#include <di/function/bind_back.h>
#include <di/function/pipeable.h>
#include <di/math/numeric_limits.h>
#include <di/meta/constexpr.h>
#include <di/meta/decay.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function {
namespace detail {
    template<typename F, size_t max_arity>
    class CurryBackFunction;
}

template<typename F, size_t max_arity = math::NumericLimits<size_t>::max>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F>)
constexpr auto curry_back(F&& function, Constexpr<max_arity> = {}) {
    return detail::CurryBackFunction<meta::Decay<F>, max_arity>(types::in_place, util::forward<F>(function));
}

namespace detail {
    template<typename F, size_t max_arity>
    class CurryBackFunction : public function::pipeline::EnablePipeline {
    private:
        template<size_t arity>
        constexpr static size_t new_arity = max_arity == math::NumericLimits<size_t>::max ? max_arity : arity;

    public:
        CurryBackFunction() = default;

        template<typename Fn>
        constexpr CurryBackFunction(types::InPlace, Fn&& function) : m_function(util::forward<Fn>(function)) {}

        constexpr CurryBackFunction(CurryBackFunction const&) = default;
        constexpr CurryBackFunction(CurryBackFunction&&) = default;

        constexpr CurryBackFunction& operator=(CurryBackFunction const&) = delete;
        constexpr CurryBackFunction& operator=(CurryBackFunction&&) = delete;

        template<typename... Args>
        requires(concepts::Invocable<F&, Args...>)
        constexpr decltype(auto) operator()(Args&&... args) & {
            return function::invoke(m_function, util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const&, Args...>)
        constexpr decltype(auto) operator()(Args&&... args) const& {
            return function::invoke(m_function, util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F &&, Args...>)
        constexpr decltype(auto) operator()(Args&&... args) && {
            return function::invoke(util::move(m_function), util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const &&, Args...>)
        constexpr decltype(auto) operator()(Args&&... args) const&& {
            return function::invoke(util::move(m_function), util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(!concepts::Invocable<F&, Args...> && concepts::ConstructibleFrom<F, F&> &&
                 (concepts::ConstructibleFrom<meta::Decay<Args>, Args> && ...) && sizeof...(Args) < max_arity)
        constexpr auto operator()(Args&&... args) & {
            return curry_back(bind_back(m_function, util::forward<Args>(args)...),
                              c_<new_arity<max_arity - sizeof...(Args)>>);
        }

        template<typename... Args>
        requires(!concepts::Invocable<F const&, Args...> && concepts::ConstructibleFrom<F, F const&> &&
                 (concepts::ConstructibleFrom<meta::Decay<Args>, Args> && ...) && sizeof...(Args) < max_arity)
        constexpr auto operator()(Args&&... args) const& {
            return curry_back(bind_back(m_function, util::forward<Args>(args)...),
                              c_<new_arity<max_arity - sizeof...(Args)>>);
        }

        template<typename... Args>
        requires(!concepts::Invocable<F &&, Args...> && concepts::ConstructibleFrom<F, F &&> &&
                 (concepts::ConstructibleFrom<meta::Decay<Args>, Args> && ...) && sizeof...(Args) < max_arity)
        constexpr auto operator()(Args&&... args) && {
            return curry_back(bind_back(util::move(m_function), util::forward<Args>(args)...),
                              c_<new_arity<max_arity - sizeof...(Args)>>);
        }

        template<typename... Args>
        requires(!concepts::Invocable<F const &&, Args...> && concepts::ConstructibleFrom<F, F const &&> &&
                 (concepts::ConstructibleFrom<meta::Decay<Args>, Args> && ...) && sizeof...(Args) < max_arity)
        constexpr auto operator()(Args&&... args) const&& {
            return curry_back(bind_back(util::move(m_function), util::forward<Args>(args)...),
                              c_<new_arity<max_arity - sizeof...(Args)>>);
        }

    private:
        F m_function;
    };
}
}
