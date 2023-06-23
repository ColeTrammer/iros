#pragma once

#include <di/container/algorithm/sum.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/prelude.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/prelude.h>
#include <di/container/view/drop.h>
#include <di/container/view/take.h>
#include <di/container/view/view_interface.h>
#include <di/function/minus.h>
#include <di/meta/constexpr.h>
#include <di/meta/maybe_const.h>
#include <di/vocab/tuple/prelude.h>
#include <di/vocab/variant/prelude.h>

// Implementation of views::concat as described in p2542r2:
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2542r2.html.
namespace di::container {
namespace detail {
    template<typename... Cons>
    using ConcatReference = meta::CommonReference<meta::ContainerReference<Cons>...>;

    template<typename... Cons>
    using ConcatValue = meta::CommonType<meta::ContainerValue<Cons>...>;

    template<typename... Cons>
    using ConcatRValue = meta::CommonReference<meta::ContainerRValue<Cons>...>;

    template<typename Ref, typename RRef, typename It>
    concept ConcatIndirectlyReadableImpl = requires(It const it) {
        static_cast<Ref>(*it);
        static_cast<RRef>(container::iterator_move(it));
    };

    template<typename... Cons>
    concept ConcatIndirectlyReadable =
        concepts::CommonReferenceWith<ConcatReference<Cons...>&&, ConcatValue<Cons...>&> &&
        concepts::CommonReferenceWith<ConcatReference<Cons...>&&, ConcatRValue<Cons...>&&> &&
        concepts::CommonReferenceWith<ConcatRValue<Cons...>&&, ConcatValue<Cons...> const&> &&
        concepts::Conjunction<ConcatIndirectlyReadableImpl<ConcatReference<Cons...>, ConcatRValue<Cons...>,
                                                           meta::ContainerIterator<Cons>>...>;

    template<typename... Cons>
    concept Concatable = requires {
        typename ConcatReference<Cons...>;
        typename ConcatValue<Cons...>;
        typename ConcatRValue<Cons...>;
    } && ConcatIndirectlyReadable<Cons...>;

    template<typename... Cons>
    concept ConcatRandomAccess =
        concepts::Conjunction<(concepts::RandomAccessContainer<Cons> && concepts::SizedContainer<Cons>) ...>;

    struct ConstantTimeReversible {
        template<typename Con>
        using Invoke = Constexpr<(concepts::BidirectionalContainer<Con> && concepts::CommonContainer<Con>) ||
                                 (concepts::SizedContainer<Con> && concepts::RandomAccessContainer<Con>)>;
    };

    template<typename... Cons>
    concept ConcatBidirectional = meta::All<meta::PopBack<meta::List<Cons...>>, ConstantTimeReversible> &&
                                  concepts::BidirectionalContainer<meta::Back<meta::List<Cons...>>>;
}

template<concepts::InputContainer... Views>
requires(sizeof...(Views) > 0 && concepts::Conjunction<concepts::View<Views>...> && detail::Concatable<Views...>)
class ConcatView : public ViewInterface<ConcatView<Views...>> {
private:
    template<bool is_const>
    class Iterator
        : public IteratorBase<
              Iterator<is_const>,
              meta::Conditional<
                  detail::ConcatRandomAccess<meta::MaybeConst<is_const, Views>...>, RandomAccessIteratorTag,
                  meta::Conditional<
                      detail::ConcatBidirectional<meta::MaybeConst<is_const, Views>...>, BidirectionalIteratorTag,
                      meta::Conditional<
                          concepts::Conjunction<concepts::ForwardContainer<meta::MaybeConst<is_const, Views>>...>,
                          ForwardIteratorTag, InputIteratorTag>>>,
              detail::ConcatValue<meta::MaybeConst<is_const, Views>...>,
              meta::CommonType<meta::ContainerSSizeType<meta::MaybeConst<is_const, Views>>...>> {
    private:
        using SSizeType = meta::CommonType<meta::ContainerSSizeType<meta::MaybeConst<is_const, Views>>...>;
        using BaseIter = Variant<meta::ContainerIterator<meta::MaybeConst<is_const, Views>>...>;
        using Reference = detail::ConcatReference<meta::MaybeConst<is_const, Views>...>;
        using RValue = detail::ConcatRValue<meta::MaybeConst<is_const, Views>...>;

        template<bool>
        friend class Iterator;
        friend class ConcatView;

        template<size_t N>
        constexpr void satisfy() {
            if constexpr (N != sizeof...(Views) - 1) {
                if (util::get<N>(m_base) == container::end(util::get<N>(m_parent->m_views))) {
                    m_base.template emplace<N + 1>(container::begin(util::get<N + 1>(m_parent->m_views)));
                    satisfy<N + 1>();
                }
            }
        }

        template<size_t N>
        constexpr void prev() {
            if constexpr (N == 0) {
                --util::get<0>(m_base);
            } else {
                if (util::get<N>(m_base) == container::begin(util::get<N>(m_parent->m_views))) {
                    using PrevView = meta::MaybeConst<is_const, meta::At<meta::List<Views...>, N - 1>>;
                    if constexpr (concepts::CommonContainer<PrevView>) {
                        m_base.template emplace<N - 1>(container::end(util::get<N - 1>(m_parent->m_views)));
                    } else {
                        m_base.template emplace<N - 1>(
                            container::next(container::begin(util::get<N - 1>(m_parent->m_views)),
                                            container::size(util::get<N - 1>(m_parent->m_views))));
                    }
                    prev<N - 1>();
                } else {
                    --util::get<N>(m_base);
                }
            }
        }

        template<size_t N>
        constexpr void advance_fwd(SSizeType offset, SSizeType steps) {
            if constexpr (N == sizeof...(Views) - 1) {
                util::get<N>(m_base) += steps;
            } else {
                auto n_size = container::size(util::get<N>(m_parent->m_views));
                if (offset + steps < static_cast<SSizeType>(n_size)) {
                    util::get<N>(m_base) += steps;
                } else {
                    m_base.template emplace<N + 1>(container::begin(util::get<N + 1>(m_parent->m_views)));
                    advance_fwd<N + 1>(0, offset + steps - n_size);
                }
            }
        }

        template<size_t N>
        constexpr void advance_bwd(SSizeType offset, SSizeType steps) {
            if constexpr (N == 0) {
                util::get<N>(m_base) -= steps;
            } else {
                if (offset >= steps) {
                    util::get<N>(m_base) -= steps;
                } else {
                    m_base.template emplace<N - 1>(container::begin(util::get<N - 1>(m_parent->m_views)) +
                                                   container::size(util::get<N - 1>(m_parent->m_views)));
                    advance_bwd<N - 1>(static_cast<SSizeType>(container::size(util::get<N - 1>(m_parent->m_views))),
                                       steps - offset);
                }
            }
        }

        template<typename... Args>
        requires(concepts::ConstructibleFrom<BaseIter, Args...>)
        constexpr explicit Iterator(meta::MaybeConst<is_const, ConcatView>* parent, Args&&... args)
            : m_parent(parent), m_base(util::forward<Args>(args)...) {}

    public:
        constexpr Iterator()
        requires(concepts::DefaultInitializable<BaseIter>)
        = default;

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const &&
                 concepts::Conjunction<
                     concepts::ConvertibleTo<meta::ContainerIterator<Views>, meta::ContainerIterator<Views const>>...>)
            : m_parent(other.m_parent), m_base(util::move(other.m_base)) {}

        constexpr decltype(auto) operator*() const {
            return visit(
                [](auto&& it) -> Reference {
                    return *it;
                },
                m_base);
        }

        constexpr void advance_one() {
            function::index_dispatch<void, sizeof...(Views)>(m_base.index(), [&]<size_t i>(Constexpr<i>) {
                ++util::get<i>(m_base);
                this->template satisfy<i>();
            });
        }

        constexpr void back_one()
        requires(detail::ConcatBidirectional<meta::MaybeConst<is_const, Views>...>)
        {
            function::index_dispatch<void, sizeof...(Views)>(m_base.index(), [&]<size_t i>(Constexpr<i>) {
                this->template prev<i>();
            });
        }

        constexpr void advance_n(SSizeType n)
        requires(detail::ConcatRandomAccess<meta::MaybeConst<is_const, Views>...>)
        {
            function::index_dispatch<void, sizeof...(Views)>(m_base.index(), [&]<size_t i>(Constexpr<i>) {
                if (n > 0) {
                    this->template advance_fwd<i>(
                        util::get<i>(m_base) - container::begin(util::get<i>(m_parent->m_views)), n);
                } else if (n < 0) {
                    this->template advance_bwd<i>(
                        util::get<i>(m_base) - container::begin(util::get<i>(m_parent->m_views)), -n);
                }
            });
        }

    private:
        constexpr friend bool operator==(Iterator const& a, Iterator const& b)
        requires(concepts::Conjunction<
                 concepts::EqualityComparable<meta::ContainerIterator<meta::MaybeConst<is_const, Views>>>...>)
        {
            return a.m_base == b.m_base;
        }

        constexpr bool at_end() const {
            constexpr auto last_index = sizeof...(Views) - 1;
            return m_base.index() == last_index &&
                   util::get<last_index>(m_base) == container::end(util::get<last_index>(m_parent->m_views));
        }

        constexpr friend bool operator==(Iterator const& a, DefaultSentinel) { return a.at_end(); }

        constexpr friend auto operator<=>(Iterator const& a, Iterator const& b)
        requires(concepts::Conjunction<
                 concepts::ThreeWayComparable<meta::ContainerIterator<meta::MaybeConst<is_const, Views>>>...>)
        {
            return a.m_base <=> b.m_base;
        }

        constexpr SSizeType distance_from(Iterator const& b) const {
            auto ai = this->m_base.index();
            auto bi = b.m_base.index();

            if (ai > bi) {
                auto sizes = apply(
                    [](auto const&... views) {
                        return Array { static_cast<SSizeType>(container::size(views))... };
                    },
                    m_parent->m_views);

                auto b_to_a = container::sum(sizes | view::drop(bi + 1) | view::take(ai - bi - 1));
                auto b_to_its_end =
                    function::index_dispatch<SSizeType, sizeof...(Views)>(bi, [&]<size_t index>(Constexpr<index>) {
                        return sizes[index] -
                               (util::get<index>(b.m_base) - container::begin(util::get<index>(m_parent->m_views)));
                    });
                auto a_to_its_start =
                    function::index_dispatch<SSizeType, sizeof...(Views)>(ai, [&]<size_t index>(Constexpr<index>) {
                        return util::get<index>(this->m_base) - container::begin(util::get<index>(m_parent->m_views));
                    });

                return b_to_its_end + b_to_a + a_to_its_start;
            } else if (ai < bi) {
                return b.distance_from(*this);
            } else {
                return function::index_dispatch<SSizeType, sizeof...(Views)>(ai, [&]<size_t index>(Constexpr<index>) {
                    return util::get<index>(this->m_base) - util::get<index>(b.m_base);
                });
            }
        }

        constexpr SSizeType distance_from_end() const {
            auto sizes = apply(
                [](auto const&... views) {
                    return Array { static_cast<SSizeType>(container::size(views))... };
                },
                m_parent->m_views);

            auto index = m_base.index();
            auto rest_to_end = container::sum(sizes | view::drop(index + 1));
            auto current_to_its_end =
                function::index_dispatch<SSizeType, sizeof...(Views)>(index, [&]<size_t index>(Constexpr<index>) {
                    return sizes[index] -
                           (util::get<index>(m_base) - container::begin(util::get<index>(m_parent->m_views)));
                });

            return -(rest_to_end + current_to_its_end);
        }

        constexpr friend SSizeType operator-(Iterator const& a, Iterator const& b)
        requires(detail::ConcatRandomAccess<meta::MaybeConst<is_const, Views>...>)
        {
            return a.distance_from(b);
        }

        constexpr friend SSizeType operator-(Iterator const& a, DefaultSentinel)
        requires(detail::ConcatRandomAccess<meta::MaybeConst<is_const, Views>...>)
        {
            return a.distance_from_end();
        }

        constexpr friend SSizeType operator-(DefaultSentinel, Iterator const& a)
        requires(detail::ConcatRandomAccess<meta::MaybeConst<is_const, Views>...>)
        {
            return -a.distance_from_end();
        }

        constexpr friend decltype(auto) tag_invoke(Iterator const& a) { return visit<RValue>(iterator_move, a.m_base); }

        template<typename = void>
        constexpr friend void tag_invoke(Iterator const& a, Iterator const& b)
        requires(requires { visit(iterator_swap, a.m_base, b.m_base); })
        {
            visit(iterator_swap, a.m_base, b.m_base);
        }

        meta::MaybeConst<is_const, ConcatView>* m_parent { nullptr };
        BaseIter m_base;
    };

public:
    ConcatView() = default;

    ConcatView()
    requires(!concepts::DefaultConstructible<Views> || ...)
    = delete;

    constexpr explicit ConcatView(Views... views) : m_views(util::move(views)...) {}

    constexpr auto begin()
    requires(!concepts::Conjunction<concepts::SimpleView<Views>...>)
    {
        auto it = Iterator<false>(this, c_<0zu>, container::begin(util::get<0>(m_views)));
        it.template satisfy<0>();
        return it;
    }

    constexpr auto begin() const
    requires(concepts::Conjunction<concepts::Container<Views const>...> && detail::Concatable<Views const...>)
    {
        auto it = Iterator<true>(this, c_<0zu>, container::begin(util::get<0>(m_views)));
        it.template satisfy<0>();
        return it;
    }

    constexpr auto end()
    requires(!concepts::Conjunction<concepts::SimpleView<Views>...>)
    {
        if constexpr (concepts::CommonContainer<meta::Back<meta::List<Views...>>>) {
            constexpr auto N = sizeof...(Views);
            return Iterator<false>(this, c_<N - 1>, container::end(util::get<N - 1>(m_views)));
        } else {
            return default_sentinel;
        }
    }

    constexpr auto end() const
    requires(concepts::Conjunction<concepts::Container<Views const>...>)
    {
        if constexpr (concepts::CommonContainer<meta::Back<meta::List<Views const...>>>) {
            constexpr auto N = sizeof...(Views);
            return Iterator<true>(this, c_<N - 1>, container::end(util::get<N - 1>(m_views)));
        } else {
            return default_sentinel;
        }
    }

    constexpr auto size()
    requires(concepts::Conjunction<concepts::SizedContainer<Views>...>)
    {
        return apply(
            [](auto... sizes) {
                using CT = meta::MakeUnsigned<meta::CommonType<decltype(sizes)...>>;
                return (CT { 0 } + ... + CT { sizes });
            },
            tuple_transform(container::size, m_views));
    }

    constexpr auto size() const
    requires(concepts::Conjunction<concepts::SizedContainer<Views const>...>)
    {
        return apply(
            [](auto... sizes) {
                using CT = meta::MakeUnsigned<meta::CommonType<decltype(sizes)...>>;
                return (CT { 0 } + ... + CT { sizes });
            },
            tuple_transform(container::size, m_views));
    }

private:
    [[no_unique_address]] Tuple<Views...> m_views;
};

template<typename... Cons>
ConcatView(Cons&&...) -> ConcatView<meta::AsView<Cons>...>;
}
