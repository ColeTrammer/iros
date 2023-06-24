#pragma once

#include <di/function/invoke.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
#include <di/util/exchange.h>
#include <di/util/forward.h>
#include <di/util/move.h>

namespace di::util {
template<concepts::InvocableTo<void> F>
class [[nodiscard]] ScopeExit {
public:
    template<typename G>
    requires(!concepts::RemoveCVRefSameAs<G, ScopeExit> && concepts::ConstructibleFrom<F, G>)
    constexpr explicit ScopeExit(G&& function) : m_function(util::forward<F>(function)) {}

    constexpr ScopeExit(ScopeExit&& other)
    requires(concepts::MoveConstructible<F>)
        : m_function(util::move(other.m_function)), m_released(util::exchange(other.m_released, true)) {}

    constexpr ~ScopeExit() {
        bool released = util::exchange(m_released, true);
        if (!released) {
            function::invoke(util::move(m_function));
        }
    }

    ScopeExit& operator=(ScopeExit&&) = delete;

    constexpr void release() { m_released = true; }

private:
    F m_function;
    bool m_released { false };
};

template<typename F>
ScopeExit(F) -> ScopeExit<F>;
}
