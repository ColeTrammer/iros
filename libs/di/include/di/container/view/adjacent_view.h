#pragma once

#include <di/container/algorithm/min.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/next.h>
#include <di/container/iterator/sentinel_base.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/range.h>
#include <di/container/view/reverse.h>
#include <di/container/view/view_interface.h>
#include <di/container/view/zip.h>
#include <di/vocab/array/prelude.h>

namespace di::container {
template<concepts::ForwardContainer View, size_t N>
requires(concepts::View<View> && N > 0)
class AdjacentView
    : public ViewInterface<AdjacentView<View, N>>
    , public meta::EnableBorrowedContainer<AdjacentView<View, N>, concepts::BorrowedContainer<View>> {
private:
    template<bool is_const>
    using Iter = meta::ContainerIterator<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    using Sent = meta::ContainerSentinel<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    using Value = meta::ContainerValue<meta::MaybeConst<is_const, View>>;

    template<bool is_const>
    using SSizeType = meta::ContainerSSizeType<meta::MaybeConst<is_const, View>>;

    struct AsSentinel {};

    template<bool is_const>
    class Sentinel;

    template<bool is_const>
    class Iterator
        : public IteratorBase<Iterator<is_const>,
                              meta::Conditional<concepts::RandomAccessIterator<Iter<is_const>>, RandomAccessIteratorTag,
                                                meta::Conditional<concepts::BidirectionalIterator<Iter<is_const>>,
                                                                  BidirectionalIteratorTag, ForwardIteratorTag>>,
                              meta::AsTuple<meta::Repeat<Value<is_const>, N>>, SSizeType<is_const>> {
    public:
        Iterator() = default;

        constexpr explicit Iterator(Iter<is_const> first, Sent<is_const> last) {
            m_iterators[0] = util::move(first);
            for (auto i : view::range(1zu, N)) {
                m_iterators[i] = container::next(m_iterators[i - 1], 1, last);
            }
        }

        constexpr explicit Iterator(AsSentinel, Iter<is_const> first, Iter<is_const> last) {
            if constexpr (concepts::BidirectionalIterator<Iter<is_const>>) {
                m_iterators[N - 1] = util::move(last);
                for (auto i : view::range(N - 1) | view::reverse) {
                    m_iterators[i] = container::prev(m_iterators[i + 1], 1, first);
                }
            } else {
                for (auto i : view::range(N)) {
                    m_iterators[i] = last;
                }
            }
        }

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Iter<false>, Iter<true>>)
        {
            for (auto& [left, right] : view::zip(this->m_iterators, other.m_iterators)) {
                left = util::move(right);
            }
        }

        constexpr auto operator*() const {
            return tuple_transform(
                [](auto& iterator) -> decltype(*iterator) {
                    return *iterator;
                },
                m_iterators);
        }

        constexpr void advance_one() {
            tuple_for_each(
                [](auto& iterator) {
                    ++iterator;
                },
                m_iterators);
        }

        constexpr void back_one()
        requires(concepts::BidirectionalIterator<Iter<is_const>>)
        {
            tuple_for_each(
                [](auto& iterator) {
                    --iterator;
                },
                m_iterators);
        }

        constexpr void advance_n(SSizeType<is_const> n)
        requires(concepts::RandomAccessIterator<Iter<is_const>>)
        {
            tuple_for_each(
                [&](auto& iterator) {
                    iterator += n;
                },
                m_iterators);
        }

        constexpr Array<Iter<is_const>, N> const& iterators() const { return m_iterators; }

    private:
        constexpr friend bool operator==(Iterator const& a, Iterator const& b)
        requires(concepts::EqualityComparable<Iter<is_const>>)
        {
            return a.m_iterators.back() == b.m_iterators.back();
        }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(concepts::RandomAccessIterator<Iter<is_const>> && concepts::ThreeWayComparable<Iter<is_const>>)
        {
            return a.m_iterators.back() <=> b.m_iterators.back();
        }

        constexpr friend SSizeType<is_const> operator-(Iterator const& a, Iterator const& b)
        requires(concepts::SizedSentinelFor<Iter<is_const>, Iter<is_const>>)
        {
            return a.m_iterators.back() - b.m_iterators.back();
        }

        template<bool other_is_const>
        friend class Sentinel;

        constexpr friend auto tag_invoke(types::Tag<iterator_move>, Iterator const& self) {
            return tuple_transform(iterator_move, self.m_iterators);
        }

        constexpr friend void tag_invoke(types::Tag<iterator_swap>, Iterator const& a, Iterator const& b)
        requires(concepts::IndirectlySwappable<Iter<is_const>>)
        {
            return function::unpack<meta::MakeIndexSequence<N>>(
                [&]<size_t... indices>(meta::IndexSequence<indices...>) {
                    return (void) (iterator_swap(util::get<indices>(a.m_iterators), util::get<indices>(b.m_iterators)),
                                   ...);
                });
        }

        Array<Iter<is_const>, N> m_iterators;
    };

    template<bool is_const>
    class Sentinel : public SentinelBase<Sentinel<is_const>> {
    public:
        Sentinel() = default;

        constexpr explicit Sentinel(Sent<is_const> sentinel) : m_sentinel(sentinel) {}

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Sent<false>, Sent<true>>)
            : m_sentinel(other.base()) {}

        constexpr auto base() const { return m_sentinel; }

        template<bool other_is_const>
        requires(concepts::SizedSentinelFor<Sent<is_const>, Iter<other_is_const>>)
        constexpr SSizeType<is_const> difference(Iterator<other_is_const> const& a) const {
            return base() - a.m_iterators.back();
        }

    private:
        template<bool other_is_const>
        requires(concepts::SentinelFor<Sent<is_const>, Iter<other_is_const>>)
        constexpr friend bool operator==(Iterator<other_is_const> const& a, Sentinel const& b) {
            return a.m_iterators.back() == b.base();
        }

        Sent<is_const> m_sentinel;
    };

public:
    AdjacentView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit AdjacentView(View base) : m_base(util::move(base)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr auto begin()
    requires(!concepts::SimpleView<View>)
    {
        return Iterator<false>(container::begin(m_base), container::end(m_base));
    }

    constexpr auto begin() const
    requires(concepts::Container<View const>)
    {
        return Iterator<true>(container::begin(m_base), container::end(m_base));
    }

    constexpr auto end()
    requires(!concepts::SimpleView<View>)
    {
        if constexpr (!concepts::CommonContainer<View>) {
            return Sentinel<false>(container::end(m_base));
        } else {
            return Iterator<false>(AsSentinel {}, container::begin(m_base), container::end(m_base));
        }
    }

    constexpr auto end() const
    requires(concepts::Container<View const>)
    {
        if constexpr (!concepts::CommonContainer<View const>) {
            return Sentinel<true>(container::end(m_base));
        } else {
            return Iterator<true>(AsSentinel {}, container::begin(m_base), container::end(m_base));
        }
    }

    constexpr auto size()
    requires(concepts::SizedContainer<View>)
    {
        using SizeType = decltype(container::size(m_base));
        using CommonType = meta::CommonType<SizeType, size_t>;
        auto size = static_cast<CommonType>(container::size(m_base));
        size -= container::min(size, static_cast<CommonType>(N - 1));
        return static_cast<SizeType>(size);
    }

    constexpr auto size() const
    requires(concepts::SizedContainer<View const>)
    {
        using SizeType = decltype(container::size(m_base));
        using CommonType = meta::CommonType<SizeType, size_t>;
        auto size = static_cast<CommonType>(container::size(m_base));
        size -= container::min(size, static_cast<CommonType>(N - 1));
        return static_cast<SizeType>(size);
    }

private:
    View m_base;
};
}
