#pragma once

#include <di/function/bind_front.h>
#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/math/numeric_limits.h>
#include <di/meta/callable.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/integers.h>

namespace di::function::curry_ns {
template<typename Self>
struct Curry;

template<typename F, usize max_arity_in = NumericLimits<usize>::max>
struct CurryImpl : Curry<CurryImpl<F, max_arity_in>> {
    using Base = Curry<CurryImpl<F, max_arity_in>>;

public:
    constexpr static auto max_arity = max_arity_in;

    CurryImpl() = default;

    template<typename... Args>
    requires(concepts::ConstructibleFrom<F, Args...>)
    constexpr CurryImpl(InPlace, Args&&... args) : m_f(di::forward<Args>(args)...) {}

    CurryImpl(CurryImpl const&) = default;
    CurryImpl(CurryImpl&&) = default;

    auto operator=(CurryImpl const&) -> CurryImpl& = delete;
    auto operator=(CurryImpl&&) -> CurryImpl& = delete;

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
struct Curry : pipeline::EnablePipeline {
    constexpr static auto max_arity() { return deduce_max_arity<Self>(); }

    Curry() = default;

    Curry(Curry const&) = default;
    Curry(Curry&&) = default;

    auto operator=(Curry const&) -> Curry& = delete;
    auto operator=(Curry&&) -> Curry& = delete;

    template<concepts::DecayConstructible... Args>
    requires(concepts::ConstructibleFrom<Self, Self&> && sizeof...(Args) < max_arity())
    constexpr auto operator()(Args&&... args) & {
        return di::bind_front(static_cast<Self&>(*this), di::forward<Args>(args)...);
    }

    template<concepts::DecayConstructible... Args>
    requires(concepts::ConstructibleFrom<Self, Self const&> && sizeof...(Args) < max_arity())
    constexpr auto operator()(Args&&... args) const& {
        return di::bind_front(static_cast<Self const&>(*this), di::forward<Args>(args)...);
    }

    template<concepts::DecayConstructible... Args>
    requires(concepts::ConstructibleFrom<Self, Self &&> && sizeof...(Args) < max_arity())
    constexpr auto operator()(Args&&... args) && {
        return di::bind_front(static_cast<Self&&>(*this), di::forward<Args>(args)...);
    }

    template<concepts::DecayConstructible... Args>
    requires(concepts::ConstructibleFrom<Self, Self const &&> && sizeof...(Args) < max_arity())
    constexpr auto operator()(Args&&... args) const&& {
        return di::bind_front(static_cast<Self const&&>(*this), di::forward<Args>(args)...);
    }
};

struct CurryFunction {
    template<concepts::DecayConstructible F>
    constexpr auto operator()(F&& function) const {
        return CurryImpl<F> { in_place, di::forward<F>(function) };
    }

    template<concepts::DecayConstructible F, usize max_arity>
    constexpr auto operator()(F&& function, Constexpr<max_arity>) const {
        return CurryImpl<F, max_arity> { in_place, di::forward<F>(function) };
    }
};
}

namespace di::function {
using curry_ns::Curry;
constexpr inline auto curry = curry_ns::CurryFunction {};
}

namespace di {
using function::curry;
using function::Curry;
}
