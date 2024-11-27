#pragma once

#include <di/meta/operations.h>
#include <di/util/exchange.h>
#include <di/util/move.h>

namespace di::util {
template<concepts::Copyable T>
class [[nodiscard]] ScopeValueChange {
public:
    constexpr explicit ScopeValueChange(T& value, T new_value) : m_value(value), m_old_value(value) {
        m_value = util::move(new_value);
    }

    constexpr ScopeValueChange(ScopeValueChange&& other)
        : m_value(other.m_value)
        , m_old_value(util::move(other.m_old_value))
        , m_released(util::exchange(other.m_released, true)) {}

    constexpr ~ScopeValueChange() { m_value = util::move(m_old_value); }

    auto operator=(ScopeValueChange&&) -> ScopeValueChange& = delete;

    constexpr void release() { m_released = true; }

private:
    T& m_value;
    T m_old_value;
    bool m_released { false };
};

template<typename T>
ScopeValueChange(T&, T) -> ScopeValueChange<T>;
}

namespace di {
using util::ScopeValueChange;
}
