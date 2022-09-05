#pragma once

#include <di/concepts/conjunction.h>
#include <di/concepts/constructible_from.h>
#include <di/function/bind_front.h>
#include <di/function/pipeable.h>
#include <di/meta/decay.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function {
namespace detail {
    template<typename F>
    class CurryFunction;
}

template<typename F>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F>)
constexpr auto curry(F&& function) {
    return detail::CurryFunction<meta::Decay<F>>(types::in_place, util::forward<F>(function));
}

template<typename Self>
class Curry : public pipeline::EnablePipeline {
public:
    template<typename... Args>
    requires(/* !concepts::Invocable<Self&, Args...> && */ concepts::ConstructibleFrom<Self, Self&> &&
             concepts::Conjunction<concepts::ConstructibleFrom<meta::Decay<Args>, Args>...>)
    constexpr auto operator()(Args&&... args) & {
        return curry_back(bind_back(static_cast<Self&>(*this), util::forward<Args>(args)...));
    }

    template<typename... Args>
    requires(/* !concepts::Invocable<Self const&, Args...> && */ concepts::ConstructibleFrom<Self, Self const&> &&
             concepts::Conjunction<concepts::ConstructibleFrom<meta::Decay<Args>, Args>...>)
    constexpr auto operator()(Args&&... args) const& {
        return curry_back(bind_back(static_cast<Self const&>(*this), util::forward<Args>(args)...));
    }

    template<typename... Args>
    requires(/* !concepts::Invocable<Self &&, Args...> && */ concepts::ConstructibleFrom<Self, Self &&> &&
             concepts::Conjunction<concepts::ConstructibleFrom<meta::Decay<Args>, Args>...>)
    constexpr auto operator()(Args&&... args) && {
        return curry_back(bind_back(static_cast<Self&&>(*this), util::forward<Args>(args)...));
    }

    template<typename... Args>
    requires(/* !concepts::Invocable<Self const &&, Args...> && */ concepts::ConstructibleFrom<Self, Self const &&> &&
             concepts::Conjunction<concepts::ConstructibleFrom<meta::Decay<Args>, Args>...>)
    constexpr auto operator()(Args&&... args) const&& {
        return curry_back(bind_back(static_cast<Self const&&>(*this), util::forward<Args>(args)...));
    }
};

namespace detail {
    template<typename F>
    class CurryFunction : public Curry<CurryFunction<F>> {
    public:
        using Curry<CurryFunction<F>>::operator();

        template<typename Fn>
        constexpr CurryFunction(types::InPlace, Fn&& function) : m_function(util::forward<Fn>(function)) {}

        constexpr CurryFunction(CurryFunction const&) = default;
        constexpr CurryFunction(CurryFunction&&) = default;

        constexpr CurryFunction& operator=(CurryFunction const&) = delete;
        constexpr CurryFunction& operator=(CurryFunction&&) = delete;

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

    private:
        F m_function;
    };
}
}
