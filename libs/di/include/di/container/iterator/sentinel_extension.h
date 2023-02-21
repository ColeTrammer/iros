#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/sentinel_base.h>
#include <di/container/meta/prelude.h>

namespace di::container {
template<typename Self, typename Sent, typename WrappedIter, concepts::Iterator Iter>
requires(concepts::SentinelFor<Sent, Iter>)
class SentinelExtension : public SentinelBase<Self> {
public:
    SentinelExtension() = default;

    constexpr explicit SentinelExtension(Sent base) : m_base(base) {}

    constexpr Sent base() const { return m_base; }

    constexpr auto difference(WrappedIter const& a) const
    requires(concepts::SizedSentinelFor<Sent, Iter>)
    {
        return this->base() - a.base();
    }

private:
    constexpr friend bool operator==(Self const& a, WrappedIter const& b) { return a.base() == b.base(); }

    Sent m_base;
};
}
