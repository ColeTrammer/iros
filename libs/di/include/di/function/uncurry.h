#pragma once

#include <di/function/invoke.h>
#include <di/function/pipeable.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/types/in_place.h>
#include <di/util/forward.h>
#include <di/util/move.h>
#include <di/vocab/tuple/apply.h>

namespace di::function {
namespace detail {
    template<typename F>
    class UncurryFunction : public pipeline::EnablePipeline {
    private:
        F m_function;

    public:
        template<typename Fn>
        constexpr UncurryFunction(types::InPlace, Fn&& function) : m_function(util::forward<Fn>(function)) {}

        constexpr UncurryFunction(UncurryFunction const&) = default;
        constexpr UncurryFunction(UncurryFunction&&) = default;

        constexpr UncurryFunction& operator=(UncurryFunction const&) = delete;
        constexpr UncurryFunction& operator=(UncurryFunction&&) = delete;

        template<concepts::TupleLike Tup>
        constexpr decltype(auto) operator()(Tup&& tuple) & {
            return vocab::apply(m_function, util::forward<Tup>(tuple));
        }

        template<concepts::TupleLike Tup>
        constexpr decltype(auto) operator()(Tup&& tuple) const& {
            return vocab::apply(m_function, util::forward<Tup>(tuple));
        }

        template<concepts::TupleLike Tup>
        constexpr decltype(auto) operator()(Tup&& tuple) && {
            return vocab::apply(util::move(m_function), util::forward<Tup>(tuple));
        }

        template<concepts::TupleLike Tup>
        constexpr decltype(auto) operator()(Tup&& tuple) const&& {
            return vocab::apply(util::move(m_function), util::forward<Tup>(tuple));
        }
    };
}

template<typename F>
requires(concepts::ConstructibleFrom<meta::Decay<F>, F>)
constexpr auto uncurry(F&& function) {
    return detail::UncurryFunction<meta::Decay<F>>(types::in_place, util::forward<F>(function));
}
}
