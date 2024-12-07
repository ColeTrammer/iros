#pragma once

#include "di/container/iterator/move_iterator.h"
#include "di/container/iterator/sentinel_base.h"
#include "di/meta/operations.h"

namespace di::container {
template<concepts::Semiregular Sent>
class MoveSentinel : public SentinelBase<MoveSentinel<Sent>> {
public:
    constexpr MoveSentinel() = default;
    constexpr explicit MoveSentinel(Sent sentinel) : m_sentinel(sentinel) {}

    template<concepts::Semiregular Other>
    requires(concepts::ConvertibleTo<Other const&, Sent>)
    constexpr MoveSentinel(MoveSentinel<Other> const& other) : m_sentinel(other.base()) {}

    template<concepts::Semiregular Other>
    requires(concepts::AssignableFrom<Sent&, Other const&>)
    constexpr auto operator=(MoveSentinel<Other> const& other) -> MoveSentinel& {
        this->m_sentinel = other.base();
        return *this;
    }

    constexpr auto base() const -> Sent { return m_sentinel; }

    template<typename It>
    requires(concepts::SizedSentinelFor<Sent, It>)
    constexpr auto difference(MoveIterator<It> const& it) const {
        return m_sentinel - it;
    }

private:
    template<typename It>
    requires(concepts::SentinelFor<Sent, It>)
    constexpr friend auto operator==(MoveSentinel const& a, MoveIterator<It> const& b) -> bool {
        return a.base() == b.base();
    }

    Sent m_sentinel {};
};
}
