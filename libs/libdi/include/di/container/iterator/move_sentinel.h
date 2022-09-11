#pragma once

#include <di/concepts/assignable_from.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/semiregular.h>

namespace di::container {
template<concepts::Semiregular Sent>
class MoveSentinel {
public:
    constexpr MoveSentinel() = default;
    constexpr explicit MoveSentinel(Sent sentinel) : m_sentinel(sentinel) {}

    template<concepts::Semiregular Other>
    requires(concepts::ConvertibleTo<Other const&, Sent>)
    constexpr MoveSentinel(MoveSentinel<Other> const& other) : m_sentinel(other.base()) {}

    template<concepts::Semiregular Other>
    requires(concepts::AssignableFrom<Sent&, Other const&>)
    constexpr MoveSentinel& operator=(MoveSentinel<Other> const& other) {
        this->m_sentinel = other.base();
        return *this;
    }

    constexpr Sent base() const { return m_sentinel; }

private:
    Sent m_sentinel {};
};
}
