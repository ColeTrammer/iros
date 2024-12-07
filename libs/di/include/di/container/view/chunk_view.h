#pragma once

#include "di/container/algorithm/min.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/distance.h"
#include "di/container/iterator/iterator_base.h"
#include "di/container/meta/prelude.h"
#include "di/container/types/prelude.h"
#include "di/container/view/take.h"
#include "di/container/view/view_interface.h"
#include "di/math/divide_round_up.h"
#include "di/math/to_unsigned.h"
#include "di/util/non_propagating_cache.h"
#include "di/util/store_if.h"

namespace di::container {
template<concepts::View View>
class ChunkView;

template<concepts::View View>
requires(concepts::InputContainer<View> && !concepts::ForwardContainer<View>)
class ChunkView<View> : public ViewInterface<ChunkView<View>> {
private:
    using Iter = meta::ContainerIterator<View>;
    using Sent = meta::ContainerIterator<View>;
    using SSizeType = meta::ContainerSSizeType<View>;

    class InnerIterator : public IteratorBase<InnerIterator, InputIteratorTag, meta::ContainerValue<View>, SSizeType> {
    private:
        friend class ChunkView;

        constexpr explicit InnerIterator(ChunkView& parent) : m_parent(util::addressof(parent)) {}

    public:
        InnerIterator(InnerIterator&&) = default;
        auto operator=(InnerIterator&&) -> InnerIterator& = default;

        constexpr auto base() const& -> Iter const& { return *m_parent->m_current; }

        constexpr auto operator*() const -> decltype(auto) { return **m_parent->m_current; }

        constexpr void advance_one() {
            ++*m_parent->m_current;
            if (*m_parent->m_current == container::end(m_parent->m_base)) {
                m_parent->m_remainder = 0;
            } else {
                --m_parent->m_remainder;
            }
        }

    private:
        constexpr auto at_end() const -> bool { return m_parent->m_remainder == 0; }

        constexpr auto distance() const -> SSizeType {
            return container::min(m_parent->m_remainder, container::end(m_parent->m_base) - *m_parent->m_current);
        }

        constexpr friend auto operator==(InnerIterator const& a, DefaultSentinel) -> bool { return a.at_end(); }

        constexpr friend auto operator-(DefaultSentinel, InnerIterator const& a) -> SSizeType
        requires(concepts::SizedSentinelFor<Sent, Iter>)
        {
            return a.distance();
        }

        constexpr friend auto operator-(InnerIterator const& a, DefaultSentinel) -> SSizeType
        requires(concepts::SizedSentinelFor<Sent, Iter>)
        {
            return -(default_sentinel - a);
        }

        ChunkView* m_parent { nullptr };
    };

    class ValueType : public ViewInterface<ValueType> {
        friend class ChunkView;

        constexpr explicit ValueType(ChunkView& parent) : m_parent(util::addressof(parent)) {}

    public:
        constexpr auto begin() const { return InnerIterator(*m_parent); }

        constexpr auto end() const { return default_sentinel; }

        constexpr auto size() const
        requires(concepts::SizedSentinelFor<Sent, Iter>)
        {
            return container::min(m_parent->m_remainder, container::end(m_parent->m_base) - *m_parent->m_current);
        }

    private:
        ChunkView* m_parent { nullptr };
    };

    class OuterIterator : public IteratorBase<OuterIterator, InputIteratorTag, ValueType, SSizeType> {
    private:
        friend class ChunkView;

        constexpr explicit OuterIterator(ChunkView& parent) : m_parent(util::addressof(parent)) {}

    public:
        OuterIterator(OuterIterator&&) = default;
        auto operator=(OuterIterator&&) -> OuterIterator& = default;

        constexpr auto operator*() const -> ValueType { return ValueType(*m_parent); }

        constexpr void advance_one() {
            container::advance(*m_parent->m_current, m_parent->m_remainder, container::end(m_parent->m_base));
            m_parent->m_remainder = m_parent->m_chunk_size;
        }

    private:
        constexpr auto at_end() const -> bool {
            return *m_parent->m_current == container::end(m_parent->m_base) && m_parent->m_remainder != 0;
        }

        constexpr auto distance() const -> SSizeType {
            auto const distance = container::end(m_parent->m_base) - *m_parent->m_current;

            if (distance < m_parent->m_remainder) {
                return distance == 0 ? 0 : 1;
            }
            return math::divide_round_up(distance - m_parent->m_remainder, m_parent->m_chunk_size) + 1;
        }

        constexpr friend auto operator==(OuterIterator const& a, DefaultSentinel) -> bool { return a.at_end(); }

        constexpr friend auto operator-(DefaultSentinel, OuterIterator const& a) -> SSizeType
        requires(concepts::SizedSentinelFor<Sent, Iter>)
        {
            return a.distance();
        }

        constexpr friend auto operator-(OuterIterator const& a, DefaultSentinel) -> SSizeType
        requires(concepts::SizedSentinelFor<Sent, Iter>)
        {
            return -(default_sentinel - a);
        }

        ChunkView* m_parent { nullptr };
    };

public:
    ChunkView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit ChunkView(View base, SSizeType chunk_size) : m_base(util::move(base)), m_chunk_size(chunk_size) {
        DI_ASSERT(chunk_size > 0);
    }

    constexpr auto base() const& -> View
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr auto base() && -> View { return util::move(m_base); }

    constexpr auto chunk_size() const { return m_chunk_size; }

    constexpr auto begin() {
        m_current = container::begin(m_base);
        m_remainder = m_chunk_size;
        return OuterIterator(*this);
    }

    constexpr auto end() const { return default_sentinel; }

    constexpr auto size()
    requires(concepts::SizedContainer<View>)
    {
        return math::to_unsigned(math::divide_round_up(container::distance(m_base), m_chunk_size));
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        return math::to_unsigned(math::divide_round_up(container::distance(m_base), m_chunk_size));
    }

private:
    View m_base {};
    SSizeType m_chunk_size { 0 };
    SSizeType m_remainder { 0 };
    util::NonPropagatingCache<Iter> m_current;
};

template<concepts::View View>
requires(concepts::ForwardContainer<View>)
class ChunkView<View>
    : public ViewInterface<ChunkView<View>>
    , public meta::EnableBorrowedContainer<ChunkView<View>, concepts::BorrowedContainer<View>> {
private:
    template<bool is_const>
    using Base = meta::MaybeConst<is_const, View>;

    template<bool is_const>
    using Iter = meta::ContainerIterator<Base<is_const>>;

    template<bool is_const>
    using Sent = meta::ContainerSentinel<Base<is_const>>;

    template<bool is_const>
    using SSizeType = meta::ContainerSSizeType<Base<is_const>>;

    template<bool is_const>
    using ValueType =
        decltype(view::take(reconstruct(in_place_type<Base<is_const>>, util::declval<Iter<is_const> const&>(),
                                        util::declval<Sent<is_const> const&>()),
                            util::declval<SSizeType<is_const> const&>()));

    template<bool is_const>
    class Iterator
        : public IteratorBase<Iterator<is_const>,
                              meta::Conditional<concepts::RandomAccessIterator<Iter<is_const>>, RandomAccessIteratorTag,
                                                meta::Conditional<concepts::BidirectionalIterator<Iter<is_const>>,
                                                                  BidirectionalIteratorTag, ForwardIteratorTag>>,
                              ValueType<is_const>, SSizeType<is_const>> {
    private:
        friend class ChunkView;

        constexpr Iterator(meta::MaybeConst<is_const, ChunkView>* parent, Iter<is_const> base,
                           SSizeType<is_const> missing = 0)
            : m_base(util::move(base))
            , m_end(container::end(parent->m_base))
            , m_chunk_size(parent->chunk_size())
            , m_missing(missing) {}

    public:
        Iterator() = default;

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Iter<false>, Iter<true>> &&
                 concepts::ConvertibleTo<Sent<false>, Sent<true>>)
            : m_base(util::move(other.m_base))
            , m_end(util::move(other.m_end))
            , m_chunk_size(other.m_chunk_size)
            , m_missing(other.m_missing) {}

        constexpr auto base() -> Iter<is_const> { return m_base; }

        constexpr auto operator*() const -> decltype(auto) {
            return view::take(reconstruct(in_place_type<Base<is_const>>, m_base, m_end), m_chunk_size);
        }

        constexpr void advance_one() { m_missing = container::advance(m_base, m_chunk_size, m_end); }

        constexpr void back_one()
        requires(concepts::BidirectionalIterator<Iter<is_const>>)
        {
            container::advance(m_base, m_missing - m_chunk_size);
            m_missing = 0;
        }

        constexpr void advance_n(SSizeType<is_const> n)
        requires(concepts::RandomAccessIterator<Iter<is_const>>)
        {
            if (n > 0) {
                m_missing = container::advance(m_base, m_chunk_size * n, m_end);
            } else if (n < 0) {
                container::advance(m_base, m_chunk_size * n + m_missing);
                m_missing = 0;
            }
        }

    private:
        template<bool>
        friend class Iterator;

        constexpr friend auto operator==(Iterator const& self, DefaultSentinel) -> bool {
            return self.m_base == self.m_end;
        }
        constexpr friend auto operator==(Iterator const& a, Iterator const& b) -> bool { return a.m_base == b.m_base; }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(concepts::RandomAccessIterator<Iter<is_const>>)
        {
            return a.m_base <=> b.m_base;
        }

        constexpr friend auto operator-(Iterator const& a, Iterator const& b) -> SSizeType<is_const>
        requires(concepts::SizedSentinelFor<Iter<is_const>, Iter<is_const>>)
        {
            auto n = a.m_base - b.m_base;
            return (n + a.m_missing - b.m_missing) / a.m_chunk_size;
        }

        constexpr friend auto operator-(DefaultSentinel, Iterator const& b) -> SSizeType<is_const> {
            return math::divide_round_up(b.m_base - b.m_current, b.m_chunk_size);
        }
        constexpr friend auto operator-(Iterator const& a, DefaultSentinel) -> SSizeType<is_const> {
            return -(default_sentinel - a);
        }

        Iter<is_const> m_base {};
        Sent<is_const> m_end {};
        SSizeType<is_const> m_chunk_size { 0 };
        SSizeType<is_const> m_missing { 0 };
    };

public:
    ChunkView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit ChunkView(View base, SSizeType<false> chunk_size)
        : m_base(util::move(base)), m_chunk_size(chunk_size) {
        DI_ASSERT(chunk_size > 0);
    }

    constexpr auto base() const& -> View
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr auto base() && -> View { return util::move(m_base); }

    constexpr auto chunk_size() const { return m_chunk_size; }

    constexpr auto begin()
    requires(!concepts::SimpleView<View>)
    {
        return Iterator<false>(this, container::begin(m_base));
    }

    constexpr auto begin() const
    requires(concepts::Container<View const>)
    {
        return Iterator<true>(this, container::begin(m_base));
    }

    constexpr auto end()
    requires(!concepts::SimpleView<View>)
    {
        if constexpr (concepts::CommonContainer<View> && concepts::SizedContainer<View>) {
            auto missing = (m_chunk_size - container::distance(m_base) % m_chunk_size) % m_chunk_size;
            return Iterator<false>(this, container::end(m_base), missing);
        } else if constexpr (concepts::CommonContainer<View> && !concepts::BidirectionalContainer<View>) {
            return Iterator<false>(this, container::end(m_base));
        } else {
            return default_sentinel;
        }
    }

    constexpr auto end() const
    requires(concepts::Container<View const>)
    {
        if constexpr (concepts::CommonContainer<View const> && concepts::SizedContainer<View const>) {
            auto missing = (m_chunk_size - container::distance(m_base) % m_chunk_size) % m_chunk_size;
            return Iterator<true>(this, container::end(m_base), missing);
        } else if constexpr (concepts::CommonContainer<View const> && !concepts::BidirectionalContainer<View const>) {
            return Iterator<true>(this, container::end(m_base));
        } else {
            return default_sentinel;
        }
    }

    constexpr auto size()
    requires(concepts::SizedContainer<View>)
    {
        return math::to_unsigned(math::divide_round_up(container::distance(m_base), m_chunk_size));
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        return math::to_unsigned(math::divide_round_up(container::distance(m_base), m_chunk_size));
    }

private:
    View m_base;
    SSizeType<false> m_chunk_size;
};

template<typename Con>
ChunkView(Con&&, meta::ContainerSSizeType<Con>) -> ChunkView<meta::AsView<Con>>;
}
