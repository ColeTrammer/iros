#pragma once

#include <di/concepts/assignable_from.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/default_initializable.h>
#include <di/concepts/equality_comparable_with.h>
#include <di/concepts/pointer.h>
#include <di/concepts/same_as.h>
#include <di/concepts/three_way_comparable_with.h>
#include <di/container/concepts/bidirectional_iterator.h>
#include <di/container/concepts/indirectly_swappable.h>
#include <di/container/concepts/random_access_iterator.h>
#include <di/container/iterator/iterator_category.h>
#include <di/container/iterator/iterator_move.h>
#include <di/container/iterator/iterator_ssize_type.h>
#include <di/container/iterator/iterator_value.h>
#include <di/container/iterator/prev.h>
#include <di/container/meta/iterator_rvalue.h>
#include <di/container/meta/iterator_ssize_type.h>
#include <di/container/types/prelude.h>

namespace di::container {
template<concepts::BidirectionalIterator Iter>
class ReverseIterator {
private:
    using SSizeType = meta::IteratorSSizeType<Iter>;

public:
    constexpr ReverseIterator()
    requires(concepts::DefaultInitializable<Iter>)
    = default;

    constexpr explicit ReverseIterator(Iter iter) : m_base(iter) {}

    template<typename U>
    requires(!concepts::SameAs<Iter, U> && concepts::ConvertibleTo<U const&, Iter>)
    constexpr ReverseIterator(ReverseIterator<U> const& other) : m_base(other.base()) {}

    template<typename U>
    requires(!concepts::SameAs<Iter, U> && concepts::ConvertibleTo<U const&, Iter> && concepts::AssignableFrom<Iter&, U const&>)
    constexpr ReverseIterator& operator=(ReverseIterator<U> const& other) {
        m_base = other.base();
        return *this;
    }

    constexpr Iter base() const { return m_base; }

    constexpr decltype(auto) operator*() const {
        auto copy = base();
        return *--copy;
    }

    constexpr auto operator->() const
    requires(concepts::Pointer<Iter> || requires(Iter const i) { i.operator->(); })
    {
        if constexpr (concepts::Pointer<Iter>) {
            return m_base - 1;
        } else {
            return container::prev(base()).operator->();
        }
    }

    constexpr decltype(auto) operator[](SSizeType index) const
    requires(concepts::RandomAccessIterator<Iter>)
    {
        return m_base[-1 - index];
    }

    constexpr ReverseIterator& operator++() {
        --m_base;
        return *this;
    }
    constexpr ReverseIterator& operator--() {
        ++m_base;
        return *this;
    }

    constexpr ReverseIterator operator++(int) { return ReverseIterator(m_base--); }
    constexpr ReverseIterator operator--(int) { return ReverseIterator(m_base++); }

    constexpr ReverseIterator& operator+=(SSizeType n)
    requires(concepts::RandomAccessIterator<Iter>)
    {
        m_base -= n;
        return *this;
    }
    constexpr ReverseIterator& operator-=(SSizeType n)
    requires(concepts::RandomAccessIterator<Iter>)
    {
        m_base += n;
        return *this;
    }

private:
    constexpr friend ReverseIterator operator+(ReverseIterator const& a, SSizeType n)
    requires(concepts::RandomAccessIterator<Iter>)
    {
        return ReverseIterator(a.base() - n);
    }

    constexpr friend ReverseIterator operator+(SSizeType n, ReverseIterator const& a)
    requires(concepts::RandomAccessIterator<Iter>)
    {
        return ReverseIterator(a.base() - n);
    }

    constexpr friend ReverseIterator operator-(ReverseIterator const& a, SSizeType n)
    requires(concepts::RandomAccessIterator<Iter>)
    {
        return ReverseIterator(a.base() + n);
    }

    constexpr friend auto tag_invoke(types::Tag<iterator_category>, types::InPlaceType<ReverseIterator>) {
        if constexpr (concepts::RandomAccessIterator<Iter>) {
            return types::RandomAccessIteratorTag {};
        } else {
            return types::BidirectionalIteratorTag {};
        }
    }
    constexpr friend meta::IteratorValue<Iter> tag_invoke(types::Tag<iterator_value>, types::InPlaceType<ReverseIterator>) {}
    constexpr friend SSizeType tag_invoke(types::Tag<iterator_ssize_type>, types::InPlaceType<ReverseIterator>) {}
    constexpr friend decltype(auto) tag_invoke(types::Tag<iterator_move>, ReverseIterator const& self)
    requires(requires { typename meta::IteratorRValue<Iter>; })
    {
        auto temp = self.base();
        return iterator_move(--temp);
    }

    template<concepts::IndirectlySwappable<Iter> Other>
    constexpr friend void tag_invoke(types::Tag<iterator_swap>, ReverseIterator const& a, ReverseIterator<Other> const& b) {
        auto t = a.base();
        auto u = b.base();
        iterator_swap(--t, --u);
    }

    Iter m_base {};
};

template<typename Iter, concepts::EqualityComparableWith<Iter> U>
constexpr bool operator==(ReverseIterator<Iter> const& a, ReverseIterator<U> const& b) {
    return a.base() == b.base();
}

template<typename Iter, concepts::ThreeWayComparableWith<Iter> U>
constexpr auto operator<=>(ReverseIterator<Iter> const& a, ReverseIterator<U> const& b) {
    return b.base() <=> a.base();
}

template<typename Iter, typename U>
constexpr auto operator-(ReverseIterator<Iter> const& a, ReverseIterator<U> const& b) -> decltype(b.base() - a.base()) {
    return b.base() - a.base();
}

template<concepts::BidirectionalIterator Iter>
constexpr auto make_reverse_iterator(Iter iter) {
    return container::ReverseIterator<Iter>(iter);
}
}
