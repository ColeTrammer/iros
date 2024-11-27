#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/iterator_category.h>
#include <di/container/meta/prelude.h>
#include <di/meta/compare.h>
#include <di/util/move.h>

namespace di::container {
template<typename Self, concepts::Iterator Iter, typename Value>
class IteratorExtension
    : public IteratorBase<
          Self,
          meta::Conditional<concepts::RandomAccessIterator<Iter>, RandomAccessIteratorTag,
                            meta::Conditional<concepts::BidirectionalIterator<Iter>, BidirectionalIteratorTag,
                                              meta::Conditional<concepts::ForwardIterator<Iter>, ForwardIteratorTag,
                                                                InputIteratorTag>>>,
          Value, meta::IteratorSSizeType<Iter>> {
private:
    using SSizeType = meta::IteratorSSizeType<Iter>;

public:
    IteratorExtension()
    requires(concepts::DefaultInitializable<Iter>)
    = default;

    constexpr explicit IteratorExtension(Iter base) : m_base(util::move(base)) {}

    constexpr auto base() const& -> Iter const& { return m_base; }
    constexpr auto base() && -> Iter { return util::move(m_base); }

    constexpr void advance_one() { ++m_base; }

    constexpr void back_one()
    requires(concepts::BidirectionalIterator<Iter>)
    {
        --m_base;
    }

    constexpr void advance_n(SSizeType n)
    requires(concepts::RandomAccessIterator<Iter>)
    {
        m_base += n;
    }

private:
    constexpr friend auto operator-(Self const& a, Self const& b) -> SSizeType
    requires(concepts::RandomAccessIterator<Iter>)
    {
        return a.base() - b.base();
    }

    constexpr friend auto operator==(Self const& a, Self const& b) -> bool
    requires(concepts::EqualityComparable<Iter>)
    {
        return a.base() == b.base();
    }

    constexpr friend auto operator<=>(Self const& a, Self const& b)
    requires(concepts::ThreeWayComparable<Iter>)
    {
        return a.base() <=> b.base();
    }

    Iter m_base;
};
}
