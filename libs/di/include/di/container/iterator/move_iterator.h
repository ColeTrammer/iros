#pragma once

#include <di/concepts/assignable_from.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/default_constructible.h>
#include <di/concepts/equality_comparable_with.h>
#include <di/concepts/three_way_comparable_with.h>
#include <di/container/concepts/indirectly_swappable.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/iterator_move.h>
#include <di/container/meta/iterator_rvalue.h>
#include <di/container/meta/iterator_ssize_type.h>
#include <di/container/types/prelude.h>
#include <di/meta/core.h>
#include <di/util/move.h>

namespace di::container {
template<concepts::InputIterator Iter>
class MoveIterator
    : public IteratorBase<
          MoveIterator<Iter>,
          meta::Conditional<concepts::RandomAccessIterator<Iter>, RandomAccessIteratorTag,
                            meta::Conditional<concepts::BidirectionalIterator<Iter>, BidirectionalIteratorTag,
                                              meta::Conditional<concepts::ForwardIterator<Iter>, ForwardIteratorTag,
                                                                InputIteratorTag>>>,
          meta::IteratorValue<Iter>, meta::IteratorSSizeType<Iter>> {
private:
    using SSizeType = meta::IteratorSSizeType<Iter>;

public:
    constexpr MoveIterator()
    requires(concepts::DefaultConstructible<Iter>)
    = default;

    constexpr explicit MoveIterator(Iter iterator) : m_iterator(util::move(iterator)) {}

    template<typename Other>
    requires(!concepts::SameAs<Iter, Other> && concepts::ConvertibleTo<Other const&, Iter>)
    constexpr MoveIterator(MoveIterator<Other> const& other) : m_iterator(other.m_iterator) {}

    template<typename Other>
    requires(!concepts::SameAs<Iter, Other> && concepts::ConvertibleTo<Other const&, Iter> &&
             concepts::AssignableFrom<Iter&, Other const&>)
    constexpr MoveIterator& operator=(MoveIterator<Other> const& other) {
        this->m_iterator = other.m_iterator;
        return *this;
    }

    constexpr Iter const& base() const& { return m_iterator; }
    constexpr Iter base() && { return util::move(m_iterator); }

    constexpr meta::IteratorRValue<Iter> operator*() const { return iterator_move(m_iterator); }

    constexpr void advance_one() { ++m_iterator; }

    constexpr void back_one()
    requires(concepts::BidirectionalIterator<Iter>)
    {
        --m_iterator;
    }

    constexpr void advance_n(SSizeType n)
    requires(concepts::RandomAccessIterator<Iter>)
    {
        m_iterator += n;
    }

private:
    template<concepts::InputIterator Other>
    friend class MoveIterator;

    constexpr friend decltype(auto) tag_invoke(types::Tag<iterator_move>, MoveIterator const& self)
    requires(requires { typename meta::IteratorRValue<Iter>; })
    {
        return iterator_move(self.base());
    }

    template<concepts::IndirectlySwappable<Iter> Other>
    constexpr friend void tag_invoke(types::Tag<iterator_swap>, MoveIterator const& a, MoveIterator<Other> const& b) {
        iterator_swap(a.base(), b.base());
    }

    Iter m_iterator {};
};

template<typename Iter, concepts::EqualityComparableWith<Iter> U>
constexpr bool operator==(MoveIterator<Iter> const& a, MoveIterator<U> const& b) {
    return a.base() == b.base();
}

template<typename Iter, concepts::ThreeWayComparableWith<Iter> U>
constexpr auto operator<=>(MoveIterator<Iter> const& a, MoveIterator<U> const& b) {
    return a.base() <=> b.base();
}

template<typename Iter, typename U>
constexpr auto operator-(MoveIterator<Iter> const& a, MoveIterator<U> const& b) -> decltype(a.base() - b.base()) {
    return a.base() - b.base();
}

template<concepts::InputIterator Iter>
constexpr auto make_move_iterator(Iter iterator) {
    return MoveIterator<Iter>(util::move(iterator));
}
}
