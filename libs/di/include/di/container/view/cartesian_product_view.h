#pragma once

#include <di/concepts/disjunction.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/meta/maybe_const.h>
#include <di/vocab/tuple/prelude.h>

namespace di::container {
namespace detail {
    template<typename First, typename... Rest>
    concept CartesianProductIsRandomAccess =
        concepts::Conjunction<concepts::RandomAccessContainer<First>,
                              (concepts::RandomAccessContainer<Rest> && concepts::SizedContainer<Rest>) ...>;

    template<typename Con>
    concept CartesianProductCommonArg =
        concepts::CommonContainer<Con> || (concepts::SizedContainer<Con> && concepts::RandomAccessContainer<Con>);

    template<typename First, typename... Rest>
    concept CartesianProductIsBidirectional =
        concepts::Conjunction<concepts::BidirectionalContainer<First>,
                              (concepts::BidirectionalContainer<Rest> && CartesianProductCommonArg<Rest>) ...>;

    template<typename First, typename... Rest>
    concept CartesianProductIsCommon = CartesianProductCommonArg<First>;

    template<typename... Cons>
    concept CartesianProductIsSized = concepts::Conjunction<concepts::SizedContainer<Cons>...>;

    template<typename First, typename... Cons>
    concept CartesianSentinelIsSized = concepts::Conjunction<
        concepts::SizedSentinelFor<meta::ContainerSentinel<First>, meta::ContainerIterator<First>>,
        concepts::SizedContainer<Cons>...>;

    template<CartesianProductCommonArg Con>
    constexpr auto cartiesian_common_arg_end(Con&& con) {
        if constexpr (concepts::CommonContainer<Con>) {
            return container::end(con);
        } else {
            return container::begin(con) + container::size(con);
        }
    }
}

template<concepts::InputContainer First, concepts::ForwardContainer... Rest>
requires(concepts::Conjunction<concepts::View<First>, concepts::View<Rest>...>)
class CartesianProductView : public ViewInterface<CartesianProductView<First, Rest...>> {
private:
    template<bool is_const>
    struct Iterator
        : public IteratorBase<
              Iterator<is_const>,
              meta::Conditional<
                  detail::CartesianProductIsRandomAccess<meta::MaybeConst<is_const, First>,
                                                         meta::MaybeConst<is_const, Rest>...>,
                  RandomAccessIteratorTag,
                  meta::Conditional<detail::CartesianProductIsBidirectional<meta::MaybeConst<is_const, First>,
                                                                            meta::MaybeConst<is_const, Rest>...>,
                                    RandomAccessIteratorTag,
                                    meta::Conditional<concepts::ForwardContainer<meta::MaybeConst<is_const, First>>,
                                                      ForwardIteratorTag, InputIteratorTag>>>,
              Tuple<meta::ContainerValue<First>, meta::ContainerValue<Rest>...>, ssize_t> {
    private:
        friend class CartesianProductView;

        using Parent = meta::MaybeConst<is_const, CartesianProductView>;

        using Storage = Tuple<meta::ContainerIterator<meta::MaybeConst<is_const, First>>,
                              meta::ContainerIterator<meta::MaybeConst<is_const, Rest>>...>;

        constexpr explicit Iterator(Parent& parent, Storage storage)
            : m_parent(util::addressof(parent)), m_iterators(util::move(storage)) {}

    public:
        Iterator() = default;

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const &&
                 concepts::Conjunction<
                     concepts::ConvertibleTo<meta::ContainerIterator<First>, meta::ContainerIterator<First const>>,
                     concepts::ConvertibleTo<meta::ContainerIterator<Rest>, meta::ContainerIterator<Rest const>>...>)
            : m_parent(other.m_parent), m_iterators(util::move(other)) {}

        constexpr auto operator*() const {
            return tuple_transform(
                [](auto& iterator) -> decltype(*iterator) {
                    return *iterator;
                },
                m_iterators);
        }

        constexpr void advance_one() { this->next(); }

        constexpr void back_one()
        requires(detail::CartesianProductIsBidirectional<meta::MaybeConst<is_const, First>,
                                                         meta::MaybeConst<is_const, Rest>...>)
        {
            this->prev();
        }

        constexpr void advance_n(ssize_t n)
        requires(detail::CartesianProductIsRandomAccess<meta::MaybeConst<is_const, First>,
                                                        meta::MaybeConst<is_const, Rest>...>)
        {
            if (n == 0) {
                return;
            }

            if (*this == m_parent->end()) {
                DI_ASSERT(n < 0);
                *this = m_parent->begin();
                advance_n(m_parent->size() + n);
            } else {
                this->advance(n);
            }
        }

    private:
        constexpr friend bool operator==(Iterator const& a, Iterator const& b) {
            return a.m_iterators == b.m_iterators;
        }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(concepts::Conjunction<
                 concepts::ThreeWayComparable<meta::ContainerIterator<meta::MaybeConst<is_const, First>>>,
                 concepts::ThreeWayComparable<meta::ContainerIterator<meta::MaybeConst<is_const, Rest>>>...>)
        {
            return a.m_iterators <=> b.m_iterators;
        }

        constexpr friend ssize_t operator-(Iterator const& a, Iterator const& b)
        requires(concepts::Conjunction<
                 concepts::SizedSentinelFor<meta::ContainerIterator<meta::MaybeConst<is_const, First>>,
                                            meta::ContainerIterator<meta::MaybeConst<is_const, First>>>,
                 concepts::SizedSentinelFor<meta::ContainerIterator<meta::MaybeConst<is_const, Rest>>,
                                            meta::ContainerIterator<meta::MaybeConst<is_const, Rest>>>...>)
        {
            return a.distance_from(b.m_iterators);
        }

        constexpr friend bool operator==(Iterator const& a, DefaultSentinel) { return a.at_end(); }

        constexpr bool at_end() const {
            return function::unpack<meta::MakeIndexSequence<1 + sizeof...(Rest)>>([&]<size_t... indices>(
                                                                                      meta::ListV<indices...>) {
                return ((util::get<indices>(m_iterators) == container::end(util::get<indices>(m_parent->m_bases))) ||
                        ...);
            });
        }

        constexpr friend auto operator-(Iterator const& a, DefaultSentinel)
        requires(
            detail::CartesianSentinelIsSized<meta::MaybeConst<is_const, First>, meta::MaybeConst<is_const, Rest>...>)
        {
            return a.distance_to_end();
        }

        constexpr auto distance_to_end() const {
            auto end_tuple = function::unpack<meta::MakeIndexSequence<1 + sizeof...(Rest)>>(
                [&]<size_t... indices>(meta::ListV<indices...>) {
                    return make_tuple(container::end(util::get<indices>(m_parent->m_bases))...);
                });
            return distance_to(end_tuple);
        }

        constexpr friend auto operator-(DefaultSentinel, Iterator const& b)
        requires(
            detail::CartesianSentinelIsSized<meta::MaybeConst<is_const, First>, meta::MaybeConst<is_const, Rest>...>)
        {
            return -(b - default_sentinel);
        }

        constexpr friend auto tag_invoke(types::Tag<iterator_move>, Iterator const& self) {
            return tuple_transform(iterator_move, self.m_iterators);
        }

        constexpr friend void tag_invoke(types::Tag<iterator_swap>, Iterator const& a, Iterator const& b)
        requires(concepts::Conjunction<
                 concepts::IndirectlySwappable<meta::ContainerIterator<meta::MaybeConst<is_const, First>>>,
                 concepts::IndirectlySwappable<meta::ContainerIterator<meta::MaybeConst<is_const, Rest>>>...>)
        {
            return function::unpack<meta::MakeIndexSequence<1 + sizeof...(Rest)>>(
                [&]<size_t... indices>(meta::ListV<indices...>) {
                    return (void) (iterator_swap(util::get<indices>(a.m_iterators), util::get<indices>(b.m_iterators)),
                                   ...);
                });
        }

        template<size_t N = sizeof...(Rest)>
        constexpr void next() {
            auto& it = util::get<N>(m_iterators);
            ++it;
            if constexpr (N > 0) {
                if (it == container::end(util::get<N>(m_parent->m_bases))) {
                    it = container::begin(util::get<N>(m_parent->m_bases));
                    this->next<N - 1>();
                }
            }
        }

        template<size_t N = sizeof...(Rest)>
        constexpr void prev() {
            auto& it = util::get<N>(m_iterators);
            if (it == container::begin(util::get<N>(m_parent->m_bases))) {
                it = detail::cartiesian_common_arg_end(util::get<N>(m_parent->m_bases));
                if constexpr (N > 0) {
                    this->prev<N - 1>();
                }
            }
            --it;
        }

        template<size_t N = sizeof...(Rest)>
        constexpr void advance(ssize_t n) {
            auto& it = util::get<N>(m_iterators);
            auto position = container::distance(container::begin(util::get<N>(m_parent->m_bases)), it);
            auto new_position = position + n;

            if constexpr (N == 0) {
                it += (new_position - position);
            } else {
                auto size = container::distance(util::get<N>(m_parent->m_bases));
                if (new_position < 0) {
                    new_position = size - (-new_position % size);
                }
                new_position %= size;
                it += (new_position - position);

                advance<N - 1>(n / size);
            }
        }

        template<size_t N = sizeof...(Rest), typename Tuple>
        constexpr ssize_t distance_from(Tuple const& a) const {
            auto distance = container::distance(util::get<N>(a), util::get<N>(m_iterators));
            if constexpr (N == 0) {
                return distance;
            } else {
                auto scale = container::distance(util::get<N>(m_parent->m_bases));
                return distance + scale * this->distance_from<N - 1>(a);
            }
        }

        Parent* m_parent { nullptr };
        Storage m_iterators;
    };

public:
    CartesianProductView() = default;

    constexpr explicit CartesianProductView(First first, Rest... bases)
        : m_bases(util::move(first), util::move(bases)...) {}

    constexpr auto begin()
    requires(concepts::Disjunction<!concepts::SimpleView<First>, !concepts::SimpleView<Rest>...>)
    {
        return Iterator<false>(*this, tuple_transform(container::begin, m_bases));
    }

    constexpr Iterator<true> begin() const
    requires(concepts::Conjunction<concepts::Container<First>, concepts::Container<Rest>...>)
    {
        return Iterator<true>(*this, tuple_transform(container::begin, m_bases));
    }

    constexpr auto end()
    requires(concepts::Disjunction<!concepts::SimpleView<First>, !concepts::SimpleView<Rest>...> &&
             detail::CartesianProductIsCommon<First, Rest...>)
    {
        auto it = Iterator<false>(*this, tuple_transform(container::begin, m_bases));
        bool is_empty = false;
        bool is_first = true;
        tuple_for_each(
            [&](auto&& base) {
                if (!is_first && container::empty(base)) {
                    is_empty = true;
                }
                is_first = false;
            },
            m_bases);
        if (!is_empty) {
            util::get<0>(it.m_iterators) = detail::cartiesian_common_arg_end(util::get<0>(m_bases));
        }
        return it;
    }

    constexpr auto end() const
    requires(detail::CartesianProductIsCommon<First const, Rest const...>)
    {
        auto it = Iterator<true>(*this, tuple_transform(container::begin, m_bases));
        bool is_empty = false;
        bool is_first = true;
        tuple_for_each(
            [&](auto&& base) {
                if (!is_first && container::empty(base)) {
                    is_empty = true;
                }
                is_first = false;
            },
            m_bases);
        if (!is_empty) {
            util::get<0>(it.m_iterators) = detail::cartiesian_common_arg_end(util::get<0>(m_bases));
        }
        return it;
    }

    constexpr auto end() const
    requires(!detail::CartesianProductIsCommon<First const, Rest const...>)
    {
        return default_sentinel;
    }

    constexpr auto size()
    requires(detail::CartesianProductIsSized<First, Rest...>)
    {
        size_t size = 1;
        tuple_for_each(
            [&](auto& view) {
                size *= container::size(view);
            },
            m_bases);
        return size;
    }

    constexpr auto size() const
    requires(detail::CartesianProductIsSized<First const, Rest const...>)
    {
        size_t size = 1;
        tuple_for_each(
            [&](auto& view) {
                size *= container::size(view);
            },
            m_bases);
        return size;
    }

private:
    Tuple<First, Rest...> m_bases;
};

template<typename... Cons>
CartesianProductView(Cons&&...) -> CartesianProductView<meta::AsView<Cons>...>;
}
