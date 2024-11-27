#pragma once

#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/in_place.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function {
namespace detail {
    template<typename F>
    class PipedFunction : public pipeline::EnablePipeline {
    public:
        template<typename Fn>
        constexpr PipedFunction(types::InPlace, Fn&& function) : m_function(util::forward<Fn>(function)) {}

        constexpr PipedFunction(PipedFunction const&) = default;
        constexpr PipedFunction(PipedFunction&&) = default;

        constexpr auto operator=(PipedFunction const&) -> PipedFunction& = delete;
        constexpr auto operator=(PipedFunction&&) -> PipedFunction& = delete;

        template<typename... Args>
        requires(concepts::Invocable<F&, Args...>)
        constexpr auto operator()(Args&&... args) & -> decltype(auto) {
            return function::invoke(m_function, util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const&, Args...>)
        constexpr auto operator()(Args&&... args) const& -> decltype(auto) {
            return function::invoke(m_function, util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F &&, Args...>)
        constexpr auto operator()(Args&&... args) && -> decltype(auto) {
            return function::invoke(util::move(m_function), util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const &&, Args...>)
        constexpr auto operator()(Args&&... args) const&& -> decltype(auto) {
            return function::invoke(util::move(m_function), util::forward<Args>(args)...);
        }

    private:
        F m_function;
    };
}

template<typename F>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F>)
constexpr auto piped(F&& function) {
    return detail::PipedFunction<meta::Decay<F>>(types::in_place, util::forward<F>(function));
}
}

namespace di {
using function::piped;
}
