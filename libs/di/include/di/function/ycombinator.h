#pragma once

#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/prelude.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::function {
namespace detail {
    template<typename F>
    requires(concepts::ConstructibleFrom<meta::Decay<F>, F>)
    struct YCombinator : pipeline::EnablePipeline {
    public:
        template<typename Fn>
        constexpr YCombinator(InPlace, Fn&& function) : m_function(util::forward<Fn>(function)) {}

        YCombinator(YCombinator const&) = default;
        YCombinator(YCombinator&&) = default;

        auto operator=(YCombinator const&) -> YCombinator& = delete;
        auto operator=(YCombinator&&) -> YCombinator& = delete;

        template<typename... Args>
        requires(concepts::Invocable<F&, YCombinator&, Args...>)
        constexpr auto operator()(Args&&... args) & -> decltype(auto) {
            return function::invoke(m_function, *this, util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const&, YCombinator const&, Args...>)
        constexpr auto operator()(Args&&... args) const& -> decltype(auto) {
            return function::invoke(m_function, *this, util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F &&, YCombinator &&, Args...>)
        constexpr auto operator()(Args&&... args) && -> decltype(auto) {
            return function::invoke(util::move(m_function), util::move(*this), util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const &&, YCombinator const &&, Args...>)
        constexpr auto operator()(Args&&... args) const&& -> decltype(auto) {
            return function::invoke(util::move(m_function), util::move(*this), util::forward<Args>(args)...);
        }

    private:
        F m_function;
    };

    struct YCombinatorFunction : pipeline::EnablePipeline {
        template<typename F>
        requires(concepts::ConstructibleFrom<meta::Decay<F>, F>)
        constexpr auto operator()(F&& function) const {
            return YCombinator<F>(in_place, util::forward<F>(function));
        }
    };
}

constexpr inline auto ycombinator = detail::YCombinatorFunction {};
}

namespace di {
using function::ycombinator;
}
