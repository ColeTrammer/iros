#pragma once

#include "di/container/algorithm/min.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/iterator_base.h"
#include "di/container/iterator/iterator_move.h"
#include "di/container/iterator/iterator_swap.h"
#include "di/container/iterator/sentinel_base.h"
#include "di/container/meta/enable_borrowed_container.h"
#include "di/container/meta/prelude.h"
#include "di/container/types/prelude.h"
#include "di/container/view/view_interface.h"
#include "di/meta/common.h"
#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/util/move.h"
#include "di/vocab/tuple/prelude.h"

namespace di::container {
template<concepts::InputContainer... Views>
requires((concepts::View<Views> && ...) && sizeof...(Views) > 0)
class ZipView
    : public ViewInterface<ZipView<Views...>>
    , public meta::EnableBorrowedContainer<ZipView<Views...>, (concepts::BorrowedContainer<Views> && ...)> {
private:
    constexpr static bool all_simple = (concepts::SimpleView<Views> && ...);
    constexpr static bool all_const = (concepts::Container<Views const> && ...);

    template<bool is_const>
    constexpr static bool all_sized = (concepts::SizedContainer<meta::MaybeConst<is_const, Views>> && ...);

    template<bool is_const>
    constexpr static bool all_forward = (concepts::ForwardContainer<meta::MaybeConst<is_const, Views>> && ...);

    template<bool is_const>
    constexpr static bool all_bidirectional =
        (concepts::BidirectionalContainer<meta::MaybeConst<is_const, Views>> && ...);

    template<bool is_const>
    constexpr static bool all_random_access =
        (concepts::RandomAccessContainer<meta::MaybeConst<is_const, Views>> && ...);

    template<bool is_const>
    constexpr static bool all_common = (concepts::CommonContainer<meta::MaybeConst<is_const, Views>> && ...);

    template<bool is_const>
    constexpr static bool is_common =
        (sizeof...(Views) == 1 && all_common<is_const>) || (!all_bidirectional<is_const> && all_common<is_const>) ||
        (all_random_access<is_const> && all_sized<is_const>);

    template<bool is_const>
    using SSizeType = meta::CommonType<meta::ContainerSSizeType<meta::MaybeConst<is_const, Views>>...>;

    template<bool is_const>
    using Value = Tuple<meta::ContainerValue<meta::MaybeConst<is_const, Views>>...>;

    template<bool is_const>
    class Sentinel;

    template<bool is_const>
    class Iterator
        : public IteratorBase<
              Iterator<is_const>,
              meta::Conditional<
                  all_random_access<is_const>, RandomAccessIteratorTag,
                  meta::Conditional<all_bidirectional<is_const>, BidirectionalIteratorTag,
                                    meta::Conditional<all_forward<is_const>, ForwardIteratorTag, InputIteratorTag>>>,
              Value<is_const>, SSizeType<is_const>> {
    private:
        using Storage = Tuple<meta::ContainerIterator<meta::MaybeConst<is_const, Views>>...>;

        constexpr explicit Iterator(Storage iterators) : m_iterators(util::move(iterators)) {}

        template<bool is_const_>
        friend class Sentinel;

        friend class ZipView;

    public:
        Iterator() = default;

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && (concepts::ConvertibleTo<meta::ContainerIterator<Views>,
                                                      meta::ContainerIterator<meta::MaybeConst<is_const, Views>>> &&
                              ...))
            : m_iterators(util::move(other)) {}

        Iterator(Iterator const&) = default;
        Iterator(Iterator&&) = default;

        auto operator=(Iterator const&) -> Iterator& = default;
        auto operator=(Iterator&&) -> Iterator& = default;

        Iterator(Iterator const&)
        requires(!all_forward<is_const>)
        = delete;
        auto operator=(Iterator const&) -> Iterator& requires(!all_forward<is_const>) = delete;

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
        requires(all_bidirectional<is_const>)
        {
            tuple_for_each(
                [](auto& iterator) {
                    --iterator;
                },
                m_iterators);
        }

        constexpr void advance_n(SSizeType<is_const> n)
        requires(all_random_access<is_const>)
        {
            tuple_for_each(
                [&](auto& iterator) {
                    iterator += n;
                },
                m_iterators);
        }

        constexpr auto iterators() const -> Storage const& { return m_iterators; }

    private:
        constexpr friend auto operator==(Iterator const& a, Iterator const& b) -> bool
        requires(concepts::EqualityComparable<meta::ContainerIterator<meta::MaybeConst<is_const, Views>>> && ...)
        {
            if constexpr (all_bidirectional<is_const>) {
                return a.m_iterators == b.m_iterators;
            } else {
                return function::unpack<meta::MakeIndexSequence<sizeof...(Views)>>(
                    [&]<size_t... indices>(meta::ListV<indices...>) {
                        return ((util::get<indices>(a.m_iterators) <= util::get<indices>(b.m_iterators)) || ...);
                    });
            }
        }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(all_random_access<is_const> &&
                 (concepts::ThreeWayComparable<meta::ContainerIterator<meta::MaybeConst<is_const, Views>>> && ...))
        {
            return a.m_iterators <=> b.m_iterators;
        }

        constexpr friend auto operator-(Iterator const& a, Iterator const& b) -> SSizeType<is_const>
        requires(concepts::SizedSentinelFor<meta::ContainerIterator<meta::MaybeConst<is_const, Views>>,
                                            meta::ContainerIterator<meta::MaybeConst<is_const, Views>>> &&
                 ...)
        {
            return function::unpack<meta::MakeIndexSequence<sizeof...(Views)>>(
                [&]<size_t... indices>(meta::ListV<indices...>) {
                    return container::min({ static_cast<SSizeType<is_const>>(util::get<indices>(a.m_iterators) -
                                                                             util::get<indices>(b.m_iterators))... });
                });
        }

        constexpr friend auto tag_invoke(types::Tag<iterator_move>, Iterator const& self) {
            return tuple_transform(iterator_move, self.m_iterators);
        }

        constexpr friend void tag_invoke(types::Tag<iterator_swap>, Iterator const& a, Iterator const& b)
        requires(concepts::IndirectlySwappable<meta::ContainerIterator<meta::MaybeConst<is_const, Views>>,
                                               meta::ContainerIterator<meta::MaybeConst<is_const, Views>>> &&
                 ...)
        {
            return function::unpack<meta::MakeIndexSequence<sizeof...(Views)>>(
                [&]<size_t... indices>(meta::ListV<indices...>) {
                    (void) (iterator_swap(util::get<indices>(a.m_iterators), util::get<indices>(b.m_iterators)), ...);
                });
        }

        Storage m_iterators;
    };

    template<bool is_const>
    class Sentinel : public SentinelBase<Sentinel<is_const>> {
    private:
        using Storage = Tuple<meta::ContainerSentinel<meta::MaybeConst<is_const, Views>>...>;

        constexpr explicit Sentinel(Storage sentinels) : m_sentinels(util::move(sentinels)) {}

        friend class ZipView;

    public:
        Sentinel() = default;

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && (concepts::ConvertibleTo<meta::ContainerSentinel<Views>,
                                                      meta::ContainerSentinel<meta::MaybeConst<is_const, Views>>> &&
                              ...))
            : m_sentinels(util::move(other)) {}

        template<bool other_is_const>
        requires(concepts::SizedSentinelFor<meta::ContainerSentinel<meta::MaybeConst<is_const, Views>>,
                                            meta::ContainerIterator<meta::MaybeConst<other_is_const, Views>>> &&
                 ...)
        constexpr auto difference(Iterator<other_is_const> const& a) const {
            return function::unpack<meta::MakeIndexSequence<sizeof...(Views)>>(
                [&]<size_t... indices>(meta::ListV<indices...>) {
                    return container::min({ static_cast<SSizeType<is_const>>(util::get<indices>(this->m_sentinels) -
                                                                             util::get<indices>(a.m_iterators))... });
                });
        }

    private:
        template<bool other_is_const>
        requires(concepts::SentinelFor<meta::ContainerSentinel<meta::MaybeConst<is_const, Views>>,
                                       meta::ContainerIterator<meta::MaybeConst<other_is_const, Views>>> &&
                 ...)
        constexpr friend auto operator==(Iterator<other_is_const> const& a, Sentinel const& b) -> bool {
            return function::unpack<meta::MakeIndexSequence<sizeof...(Views)>>(
                [&]<size_t... indices>(meta::ListV<indices...>) {
                    return ((util::get<indices>(a.m_iterators) == util::get<indices>(b.m_sentinels)) || ...);
                });
        }

        Storage m_sentinels;
    };

public:
    ZipView() = default;

    ZipView()
    requires(!concepts::DefaultConstructible<Views> || ...)
    = delete;

    constexpr explicit ZipView(Views... views) : m_views(util::move(views)...) {}

    constexpr auto begin()
    requires(!all_simple)
    {
        return Iterator<false>(tuple_transform(container::begin, m_views));
    }

    constexpr auto begin() const
    requires(all_const)
    {
        return Iterator<true>(tuple_transform(container::begin, m_views));
    }

    constexpr auto end()
    requires(!all_simple)
    {
        if constexpr (!is_common<false>) {
            return Sentinel<false>(tuple_transform(container::end, m_views));
        } else if constexpr (all_random_access<false>) {
            return begin() + static_cast<SSizeType<false>>(size());
        } else {
            return Iterator<false>(tuple_transform(container::end, m_views));
        }
    }

    constexpr auto end() const
    requires(all_const)
    {
        if constexpr (!is_common<true>) {
            return Sentinel<true>(tuple_transform(container::end, m_views));
        } else if constexpr (all_random_access<true>) {
            return begin() + static_cast<SSizeType<true>>(size());
        } else {
            return Iterator<true>(tuple_transform(container::end, m_views));
        }
    }

    constexpr auto size()
    requires(all_sized<false>)
    {
        return apply(
            [](auto... sizes) {
                using CommonType = meta::MakeUnsigned<meta::CommonType<decltype(sizes)...>>;
                return container::min({ static_cast<CommonType>(sizes)... });
            },
            tuple_transform(container::size, m_views));
    }

    constexpr auto size() const
    requires(all_sized<true>)
    {
        return apply(
            [](auto... sizes) {
                using CommonType = meta::MakeUnsigned<meta::CommonType<decltype(sizes)...>>;
                return container::min({ static_cast<CommonType>(sizes)... });
            },
            tuple_transform(container::size, m_views));
    }

private:
    Tuple<Views...> m_views;
};

template<typename... Cons>
ZipView(Cons&&...) -> ZipView<meta::AsView<Cons>...>;
}
