#pragma once

#include <di/concepts/dereferenceable.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/default_sentinel.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/iterator_move.h>
#include <di/container/iterator/iterator_swap.h>
#include <di/container/meta/prelude.h>
#include <di/types/prelude.h>
#include <di/util/move.h>
#include <di/util/to_address.h>

namespace di::container {
template<concepts::Iterator Iter>
class CountedIterator
    : public IteratorBase<CountedIterator<Iter>, meta::IteratorCategory<Iter>, meta::IteratorValue<Iter>,
                          meta::IteratorSSizeType<Iter>> {
private:
    using SSizeType = meta::IteratorSSizeType<Iter>;

public:
    CountedIterator()
    requires(concepts::DefaultInitializable<Iter>)
    = default;

    CountedIterator(CountedIterator const&) = default;
    CountedIterator(CountedIterator&&) = default;

    CountedIterator& operator=(CountedIterator const&) = default;
    CountedIterator& operator=(CountedIterator&&) = default;

    CountedIterator(CountedIterator const&)
    requires(!concepts::ForwardIterator<Iter>)
    = delete;
    CountedIterator& operator=(CountedIterator const&)
    requires(!concepts::ForwardIterator<Iter>)
    = delete;

    constexpr CountedIterator(Iter iterator, SSizeType n) : m_iterator(util::move(iterator)), m_count(n) {}

    template<typename It>
    requires(concepts::ConvertibleTo<It const&, Iter>)
    constexpr CountedIterator(CountedIterator<It> const& other) : m_iterator(other.base()), m_count(other.count()) {}

    constexpr Iter const& base() const& { return m_iterator; }
    constexpr Iter base() && { return util::move(m_iterator); }

    constexpr SSizeType count() const { return m_count; }

    constexpr decltype(auto) operator*() {
        DI_ASSERT_GT(count(), 0);
        return *m_iterator;
    }
    constexpr decltype(auto) operator*() const
    requires(concepts::Dereferenceable<Iter const>)
    {
        DI_ASSERT_GT(count(), 0);
        return *m_iterator;
    }

    constexpr auto operator->() const
    requires(concepts::ContiguousIterator<Iter>)
    {
        return util::to_address(m_iterator);
    }

    constexpr void advance_one() {
        ++m_iterator;
        --m_count;
    }

    constexpr void back_one()
    requires(concepts::BidirectionalIterator<Iter>)
    {
        --m_iterator;
        ++m_count;
    }

    constexpr void advance_n(SSizeType n)
    requires(concepts::RandomAccessIterator<Iter>)
    {
        m_iterator += n;
        m_count -= n;
    }

private:
    constexpr friend bool operator==(CountedIterator const& a, CountedIterator const& b) {
        return a.count() == b.count();
    }

    constexpr friend bool operator==(CountedIterator const& a, DefaultSentinel) { return a.count() == 0; }

    constexpr friend strong_ordering operator<=>(CountedIterator const& a, CountedIterator const& b) {
        return a.count() <=> b.count();
    }

    constexpr friend SSizeType operator-(CountedIterator const& a, CountedIterator const& b) {
        return b.count() - a.count();
    }

    constexpr friend SSizeType operator-(CountedIterator const& a, DefaultSentinel) { return -a.count(); }

    constexpr friend SSizeType operator-(DefaultSentinel, CountedIterator const& b) { return b.count(); }

    constexpr friend decltype(auto) tag_invoke(types::Tag<iterator_move>, CountedIterator const& self)
    requires(concepts::InputIterator<Iter>)
    {
        DI_ASSERT_GT(self.count(), 0);
        return iterator_move(self.base());
    }

    template<concepts::IndirectlySwappable<Iter> It>
    constexpr friend void tag_invoke(types::Tag<iterator_swap>, CountedIterator const& a,
                                     CountedIterator<It> const& b) {
        DI_ASSERT_GT(a.count(), 0);
        DI_ASSERT_GT(b.count(), 0);
        iterator_swap(a.base(), b.base());
    }

    Iter m_iterator;
    SSizeType m_count;
};
}
