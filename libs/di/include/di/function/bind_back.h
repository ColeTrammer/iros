#pragma once

#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/meta/algorithm.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/in_place.h>
#include <di/types/size_t.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/util/move.h>
#include <di/vocab/tuple/prelude.h>

namespace di::function {
namespace detail {
    template<typename Indices, typename F, typename... BoundArgs>
    class BindBackFunction;

    template<types::size_t... indices, typename F, typename... BoundArgs>
    class BindBackFunction<meta::ListV<indices...>, F, BoundArgs...> : public pipeline::EnablePipeline {
    public:
        template<typename Fun, typename... Args>
        constexpr BindBackFunction(types::InPlace, Fun&& function, Args&&... bound_arguments)
            : m_function(util::forward<Fun>(function)), m_bound_arguments(util::forward<Args>(bound_arguments)...) {}

        constexpr BindBackFunction(BindBackFunction const&) = default;
        constexpr BindBackFunction(BindBackFunction&&) = default;

        auto operator=(BindBackFunction const&) -> BindBackFunction& = delete;
        auto operator=(BindBackFunction&&) -> BindBackFunction& = delete;

        template<typename... Args>
        requires(concepts::Invocable<F&, Args..., BoundArgs&...>)
        constexpr auto operator()(Args&&... args) & -> decltype(auto) {
            return function::invoke(m_function, util::forward<Args>(args)..., util::get<indices>(m_bound_arguments)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const&, Args..., BoundArgs const&...>)
        constexpr auto operator()(Args&&... args) const& -> decltype(auto) {
            return function::invoke(m_function, util::forward<Args>(args)..., util::get<indices>(m_bound_arguments)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F &&, Args..., BoundArgs && ...>)
        constexpr auto operator()(Args&&... args) && -> decltype(auto) {
            return function::invoke(util::move(m_function), util::forward<Args>(args)...,
                                    util::get<indices>(util::move(m_bound_arguments))...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const &&, Args..., BoundArgs const && ...>)
        constexpr auto operator()(Args&&... args) const&& -> decltype(auto) {
            return function::invoke(util::move(m_function), util::forward<Args>(args)...,
                                    util::get<indices>(util::move(m_bound_arguments))...);
        }

    private:
        F m_function;
        vocab::Tuple<BoundArgs...> m_bound_arguments;
    };
}

template<typename F, typename... Args>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F> &&
         (concepts::ConstructibleFrom<meta::Decay<Args>, Args> && ...))
constexpr auto bind_back(F&& f, Args&&... args) {
    return detail::BindBackFunction<meta::IndexSequenceFor<Args...>, meta::Decay<F>, meta::Decay<Args>...>(
        types::in_place, util::forward<F>(f), util::forward<Args>(args)...);
}
}

namespace di {
using function::bind_back;
}
