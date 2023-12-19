#pragma once

#include <di/function/bind_back.h>
#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/math/numeric_limits.h>
#include <di/meta/callable.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/integers.h>
namespace di::function::curry_back_ns {
template<typename Self>
struct CurryBack;

template<typename F, usize max_arity_in = NumericLimits<usize>::max>
struct CurryBackImpl : CurryBack<CurryBackImpl<F, max_arity_in>> {
    using Base = CurryBack<CurryBackImpl<F, max_arity_in>>;

public:
    constexpr static auto max_arity = max_arity_in;

    CurryBackImpl() = default;

    template<typename... Args>
    requires(concepts::ConstructibleFrom<F, Args...>)
    constexpr CurryBackImpl(InPlace, Args&&... args) : m_f(di::forward<Args>(args)...) {}

    CurryBackImpl(CurryBackImpl const&) = default;
    CurryBackImpl(CurryBackImpl&&) = default;

    CurryBackImpl& operator=(CurryBackImpl const&) = delete;
    CurryBackImpl& operator=(CurryBackImpl&&) = delete;

    template<typename... Args>
    requires(concepts::Invocable<F&, Args...> || concepts::Callable<Base&, Args...>)
    constexpr auto operator()(Args&&... args) & -> decltype(auto) {
        if constexpr (concepts::Invocable<F&, Args...>) {
            return di::invoke(m_f, di::forward<Args>(args)...);
        } else {
            return this->Base::operator()(di::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    requires(concepts::Invocable<F const&, Args...> || concepts::Callable<Base const&, Args...>)
    constexpr auto operator()(Args&&... args) const& -> decltype(auto) {
        if constexpr (concepts::Invocable<F const&, Args...>) {
            return di::invoke(m_f, di::forward<Args>(args)...);
        } else {
            return this->Base::operator()(di::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    requires(concepts::Invocable<F, Args...> || concepts::Callable<Base &&, Args...>)
    constexpr auto operator()(Args&&... args) && -> decltype(auto) {
        if constexpr (concepts::Invocable<F, Args...>) {
            return di::invoke(di::move(m_f), di::forward<Args>(args)...);
        } else {
            return di::move(*this).Base::operator()(di::forward<Args>(args)...);
        }
    }

    template<typename... Args>
    requires(concepts::Invocable<F const &&, Args...> || concepts::Callable<Base const &&, Args...>)
    constexpr auto operator()(Args&&... args) const&& -> decltype(auto) {
        if constexpr (concepts::Invocable<F const&&, Args...>) {
            return di::invoke(di::move(m_f), di::forward<Args>(args)...);
        } else {
            return di::move(*this).Base::operator()(di::forward<Args>(args)...);
        }
    }

private:
    F m_f {};
};

template<typename F>
constexpr auto deduce_max_arity() {
    if constexpr (requires { F::max_arity; }) {
        return F::max_arity;
    } else {
        return NumericLimits<usize>::max;
    }
}

template<typename Self>
struct CurryBack : pipeline::EnablePipeline {
    constexpr static auto max_arity = deduce_max_arity<Self>();

    CurryBack() = default;

    CurryBack(CurryBack const&) = default;
    CurryBack(CurryBack&&) = default;

    CurryBack& operator=(CurryBack const&) = delete;
    CurryBack& operator=(CurryBack&&) = delete;

    template<concepts::DecayConstructible... Args>
    requires(concepts::ConstructibleFrom<Self, Self&> && sizeof...(Args) < max_arity)
    constexpr auto operator()(Args&&... args) & {
        return di::bind_back(static_cast<Self&>(*this), di::forward<Args>(args)...);
    }

    template<concepts::DecayConstructible... Args>
    requires(concepts::ConstructibleFrom<Self, Self const&> && sizeof...(Args) < max_arity)
    constexpr auto operator()(Args&&... args) const& {
        return di::bind_back(static_cast<Self const&>(*this), di::forward<Args>(args)...);
    }

    template<concepts::DecayConstructible... Args>
    requires(concepts::ConstructibleFrom<Self, Self &&> && sizeof...(Args) < max_arity)
    constexpr auto operator()(Args&&... args) && {
        return di::bind_back(static_cast<Self&&>(*this), di::forward<Args>(args)...);
    }

    template<concepts::DecayConstructible... Args>
    requires(concepts::ConstructibleFrom<Self, Self const &&> && sizeof...(Args) < max_arity)
    constexpr auto operator()(Args&&... args) const&& {
        return di::bind_back(static_cast<Self const&&>(*this), di::forward<Args>(args)...);
    }
};

struct CurryBackFunction {
    template<concepts::DecayConstructible F>
    constexpr auto operator()(F&& function) const {
        return CurryBackImpl<F> { in_place, di::forward<F>(function) };
    }

    template<concepts::DecayConstructible F, usize max_arity>
    constexpr auto operator()(F&& function, Constexpr<max_arity>) const {
        return CurryBackImpl<F, max_arity> { in_place, di::forward<F>(function) };
    }
};
}

namespace di::function {
using curry_back_ns::CurryBack;
constexpr inline auto curry_back = curry_back_ns::CurryBackFunction {};
}

namespace di {
using function::curry_back;
using function::CurryBack;
}
