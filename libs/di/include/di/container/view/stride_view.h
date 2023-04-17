#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/advance.h>
#include <di/container/iterator/default_sentinel.h>
#include <di/container/iterator/distance.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/iterator_move.h>
#include <di/container/iterator/iterator_swap.h>
#include <di/container/meta/enable_borrowed_container.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/math/divide_round_up.h>
#include <di/math/to_unsigned.h>
#include <di/meta/maybe_const.h>

namespace di::container {
template<concepts::InputContainer View>
requires(concepts::View<View>)
class StrideView
    : public ViewInterface<StrideView<View>>
    , public meta::EnableBorrowedContainer<StrideView<View>, concepts::BorrowedContainer<View>> {
private:
    template<bool is_const>
    using Iter = meta::ContainerIterator<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    using Sent = meta::ContainerSentinel<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    using SSizeType = meta::ContainerSSizeType<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    using ValueType = meta::ContainerValue<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    class Iterator
        : public IteratorBase<
              Iterator<is_const>,
              meta::Conditional<
                  concepts::RandomAccessIterator<Iter<is_const>>, RandomAccessIteratorTag,
                  meta::Conditional<concepts::BidirectionalIterator<Iter<is_const>>, BidirectionalIteratorTag,
                                    meta::Conditional<concepts::ForwardIterator<Iter<is_const>>, ForwardIteratorTag,
                                                      InputIteratorTag>>>,
              ValueType<is_const>, SSizeType<is_const>> {
    private:
        friend class StrideView;

        constexpr Iterator(meta::MaybeConst<is_const, StrideView>* parent, Iter<is_const> base,
                           SSizeType<is_const> missing = 0)
            : m_base(util::move(base))
            , m_end(container::end(parent->m_base))
            , m_stride(parent->stride())
            , m_missing(missing) {}

    public:
        Iterator()
        requires(concepts::DefaultInitializable<Iter<is_const>>)
        = default;

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Iter<false>, Iter<true>> &&
                 concepts::ConvertibleTo<Sent<false>, Sent<true>>)
            : m_base(util::move(other.m_base))
            , m_end(util::move(other.m_end))
            , m_stride(other.m_stride)
            , m_missing(other.m_missing) {}

        Iterator(Iterator const&) = default;
        Iterator(Iterator&&) = default;

        Iterator& operator=(Iterator const&) = default;
        Iterator& operator=(Iterator&&) = default;

        Iterator(Iterator const&)
        requires(!concepts::ForwardIterator<Iter<is_const>>)
        = delete;
        Iterator& operator=(Iterator const&)
        requires(!concepts::ForwardIterator<Iter<is_const>>)
        = delete;

        constexpr Iter<is_const> base() && { return util::move(m_base); }
        constexpr Iter<is_const> const& base() const& { return m_base; }

        constexpr decltype(auto) operator*() const { return *m_base; }

        constexpr void advance_one() { m_missing = container::advance(m_base, m_stride, m_end); }

        constexpr void back_one()
        requires(concepts::BidirectionalIterator<Iter<is_const>>)
        {
            container::advance(m_base, m_missing - m_stride);
            m_missing = 0;
        }

        constexpr void advance_n(SSizeType<is_const> n)
        requires(concepts::RandomAccessIterator<Iter<is_const>>)
        {
            if (n > 0) {
                m_missing = container::advance(m_base, m_stride * n, m_end);
            } else if (n < 0) {
                container::advance(m_base, m_stride * n + m_missing);
                m_missing = 0;
            }
        }

    private:
        template<bool other_is_const>
        friend class Iterator;

        constexpr friend bool operator==(Iterator const& self, DefaultSentinel) { return self.m_base == self.m_end; }
        constexpr friend bool operator==(Iterator const& a, Iterator const& b)
        requires(concepts::EqualityComparable<Iter<is_const>>)
        {
            return a.m_base == b.m_base;
        }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(concepts::RandomAccessIterator<Iter<is_const>>)
        {
            return a.m_base <=> b.m_base;
        }

        constexpr friend SSizeType<is_const> operator-(Iterator const& a, Iterator const& b)
        requires(concepts::SizedSentinelFor<Iter<is_const>, Iter<is_const>>)
        {
            auto n = a.m_base - b.m_base;
            if constexpr (concepts::ForwardIterator<Iter<is_const>>) {
                return (n + a.m_missing - b.m_missing) / a.m_stride;
            } else if (n < 0) {
                return -math::divide_round_up(-n, a.m_stride);
            } else {
                return math::divide_round_up(n, a.m_stride);
            }
        }

        constexpr friend SSizeType<is_const> operator-(DefaultSentinel, Iterator const& b) {
            return math::divide_round_up(b.m_base - b.m_current, b.m_stride);
        }
        constexpr friend SSizeType<is_const> operator-(Iterator const& a, DefaultSentinel) {
            return -(default_sentinel - a);
        }

        constexpr friend meta::ContainerRValue<meta::MaybeConst<is_const, View>>
        tag_invoke(types::Tag<container::iterator_move>, Iterator const& a) {
            return container::iterator_move(a);
        }

        constexpr friend void tag_invoke(types::Tag<container::iterator_swap>, Iterator const& a, Iterator const& b)
        requires(concepts::IndirectlySwappable<Iter<is_const>>)
        {
            return container::iterator_swap(a, b);
        }

        Iter<is_const> m_base {};
        Sent<is_const> m_end {};
        SSizeType<is_const> m_stride { 0 };
        SSizeType<is_const> m_missing { 0 };
    };

public:
    constexpr explicit StrideView(View base, SSizeType<false> stride) : m_base(util::move(base)), m_stride(stride) {
        DI_ASSERT(stride > 0);
    }

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr auto stride() const { return m_stride; }

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
        if constexpr (concepts::CommonContainer<View> && concepts::SizedContainer<View> &&
                      concepts::ForwardContainer<View>) {
            auto missing = (m_stride - container::distance(m_base) % m_stride) % m_stride;
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
        if constexpr (concepts::CommonContainer<View const> && concepts::SizedContainer<View const> &&
                      concepts::ForwardContainer<View const>) {
            auto missing = (m_stride - container::distance(m_base) % m_stride) % m_stride;
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
        return math::to_unsigned(math::divide_round_up(container::distance(m_base), m_stride));
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        return math::to_unsigned(math::divide_round_up(container::distance(m_base), m_stride));
    }

private:
    View m_base;
    SSizeType<false> m_stride;
};

template<typename Con>
StrideView(Con&&, meta::ContainerSSizeType<Con>) -> StrideView<meta::AsView<Con>>;
}
