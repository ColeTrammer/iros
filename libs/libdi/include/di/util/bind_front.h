#pragma once

#include <di/concepts/conjunction.h>
#include <di/concepts/constructible_from.h>
#include <di/meta/decay.h>
#include <di/meta/index_sequence.h>
#include <di/meta/index_sequence_for.h>
#include <di/types/in_place.h>
#include <di/types/size_t.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/util/invoke.h>
#include <di/util/move.h>
#include <di/vocab/tuple.h>

namespace di::util {
namespace detail {
    template<typename Indices, typename F, typename... BoundArgs>
    class BindFrontFunction;

    template<types::size_t... indices, typename F, typename... BoundArgs>
    class BindFrontFunction<meta::IndexSequence<indices...>, F, BoundArgs...> {
    public:
        template<typename Fun, typename... Args>
        constexpr BindFrontFunction(types::InPlace, Fun&& function, Args&&... bound_arguments)
            : m_function(util::forward<Fun>(function)), m_bound_arguments(util::forward<Args>(bound_arguments)...) {}

        constexpr BindFrontFunction(BindFrontFunction const&) = default;
        constexpr BindFrontFunction(BindFrontFunction&&) = default;

        BindFrontFunction& operator=(BindFrontFunction const&) = delete;
        BindFrontFunction& operator=(BindFrontFunction&&) = delete;

        template<typename... Args>
        requires(concepts::Invocable<F&, BoundArgs&..., Args...>)
        constexpr decltype(auto) operator()(Args&&... args) & {
            return util::invoke(m_function, util::get<indices>(m_bound_arguments)..., util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const&, BoundArgs const&..., Args...>)
        constexpr decltype(auto) operator()(Args&&... args) const& {
            return util::invoke(m_function, util::get<indices>(m_bound_arguments)..., util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F &&, BoundArgs && ..., Args...>)
        constexpr decltype(auto) operator()(Args&&... args) && {
            return util::invoke(util::move(m_function), util::get<indices>(util::move(m_bound_arguments))..., util::forward<Args>(args)...);
        }

        template<typename... Args>
        requires(concepts::Invocable<F const &&, BoundArgs const && ..., Args...>)
        constexpr decltype(auto) operator()(Args&&... args) const&& {
            return util::invoke(util::move(m_function), util::get<indices>(util::move(m_bound_arguments))..., util::forward<Args>(args)...);
        }

    private:
        F m_function;
        vocab::Tuple<BoundArgs...> m_bound_arguments;
    };
}

template<typename F, typename... Args>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F> && concepts::Conjunction<concepts::ConstructibleFrom<meta::Decay<Args>, Args>...>)
constexpr auto bind_front(F&& f, Args&&... args) {
    return detail::BindFrontFunction<meta::IndexSequenceFor<Args...>, meta::Decay<F>, meta::Decay<Args>...>(
        types::in_place, util::forward<F>(f), util::forward<Args>(args)...);
}
}
