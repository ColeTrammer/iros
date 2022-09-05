#pragma once

#include <di/concepts/conjunction.h>
#include <di/concepts/constructible_from.h>
#include <di/function/bind_back.h>
#include <di/function/pipeable.h>
#include <di/meta/decay.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function {
namespace detail {
    template<typename F>
    class CurryBackFunction;
}

template<typename F>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F>)
constexpr auto curry_back(F&& function) {
    return detail::CurryBackFunction<meta::Decay<F>>(types::in_place, util::forward<F>(function));
}

template<typename Self>
class CurryBack : public pipeline::EnablePipeline {
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
    class CurryBackFunction : public CurryBack<CurryBackFunction<F>> {
    public:
        using CurryBack<CurryBackFunction<F>>::operator();

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

    private:
        F m_function;
    };
}
}
