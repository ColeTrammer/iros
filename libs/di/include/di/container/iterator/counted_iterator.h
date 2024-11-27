#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/default_sentinel.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/iterator_move.h>
#include <di/container/iterator/iterator_swap.h>
#include <di/container/meta/prelude.h>
#include <di/meta/operations.h>
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

    auto operator=(CountedIterator const&) -> CountedIterator& = default;
    auto operator=(CountedIterator&&) -> CountedIterator& = default;

    CountedIterator(CountedIterator const&)
    requires(!concepts::ForwardIterator<Iter>)
    = delete;
    auto operator=(CountedIterator const&) -> CountedIterator& requires(!concepts::ForwardIterator<Iter>) = delete;

    constexpr CountedIterator(Iter iterator, SSizeType n) : m_iterator(util::move(iterator)), m_count(n) {}

    template<typename It>
    requires(concepts::ConvertibleTo<It const&, Iter>)
    constexpr CountedIterator(CountedIterator<It> const& other) : m_iterator(other.base()), m_count(other.count()) {}

    constexpr auto base() const& -> Iter const& { return m_iterator; }
    constexpr auto base() && -> Iter { return util::move(m_iterator); }

    constexpr auto count() const -> SSizeType { return m_count; }

    constexpr auto operator*() -> decltype(auto) {
        DI_ASSERT(count() > 0);
        return *m_iterator;
    }
    constexpr auto operator*() const -> decltype(auto)
    requires(concepts::Dereferenceable<Iter const>)
    {
        DI_ASSERT(count() > 0);
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
    constexpr friend auto operator==(CountedIterator const& a, CountedIterator const& b) -> bool {
        return a.count() == b.count();
    }

    constexpr friend auto operator==(CountedIterator const& a, DefaultSentinel) -> bool { return a.count() == 0; }

    constexpr friend auto operator<=>(CountedIterator const& a, CountedIterator const& b) -> strong_ordering {
        return a.count() <=> b.count();
    }

    constexpr friend auto operator-(CountedIterator const& a, CountedIterator const& b) -> SSizeType {
        return b.count() - a.count();
    }

    constexpr friend auto operator-(CountedIterator const& a, DefaultSentinel) -> SSizeType { return -a.count(); }

    constexpr friend auto operator-(DefaultSentinel, CountedIterator const& b) -> SSizeType { return b.count(); }

    constexpr friend auto tag_invoke(types::Tag<iterator_move>, CountedIterator const& self) -> decltype(auto)
    requires(concepts::InputIterator<Iter>)
    {
        DI_ASSERT(self.count() > 0);
        return iterator_move(self.base());
    }

    template<concepts::IndirectlySwappable<Iter> It>
    constexpr friend void tag_invoke(types::Tag<iterator_swap>, CountedIterator const& a,
                                     CountedIterator<It> const& b) {
        DI_ASSERT(a.count() > 0);
        DI_ASSERT(b.count() > 0);
        iterator_swap(a.base(), b.base());
    }

    Iter m_iterator;
    SSizeType m_count;
};
}
