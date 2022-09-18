#pragma once

#include <di/concepts/equality_comparable.h>
#include <di/concepts/three_way_comparable.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/iterator_category.h>
#include <di/container/meta/prelude.h>
#include <di/util/move.h>

namespace di::container {
template<typename Self, concepts::Iterator Iter, typename Value>
class IteratorExtension : public IteratorBase<Self, Value, meta::IteratorSSizeType<Iter>> {
private:
    using SSizeType = meta::IteratorSSizeType<Iter>;

public:
    IteratorExtension()
    requires(concepts::DefaultInitializable<Iter>)
    = default;

    constexpr explicit IteratorExtension(Iter base) : m_base(util::move(base)) {}

    constexpr Iter const& base() const& { return m_base; }
    constexpr Iter base() && { return util::move(m_base); }

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
    constexpr friend SSizeType operator-(Self const& a, Self const& b)
    requires(concepts::RandomAccessIterator<Iter>)
    {
        return a.base() - b.base();
    }

    constexpr friend bool operator==(Self const& a, Self const& b)
    requires(concepts::EqualityComparable<Iter>)
    {
        return a.base() == b.base();
    }

    constexpr friend auto operator<=>(Self const& a, Self const& b)
    requires(concepts::ThreeWayComparable<Iter>)
    {
        return a.base() <=> b.base();
    }

    constexpr friend auto tag_invoke(types::Tag<iterator_category>, InPlaceType<Self>) {
        if constexpr (concepts::RandomAccessIterator<Iter>) {
            return types::RandomAccessIteratorTag {};
        } else if constexpr (concepts::BidirectionalIterator<Iter>) {
            return types::BidirectionalIteratorTag {};
        } else if constexpr (concepts::ForwardIterator<Iter>) {
            return types::ForwardIteratorTag {};
        } else {
            return types::InputIteratorTag {};
        }
    }

    Iter m_base;
};
}