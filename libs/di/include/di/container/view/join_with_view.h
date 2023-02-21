#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/interface/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/all.h>
#include <di/container/view/single_view.h>
#include <di/container/view/view_interface.h>
#include <di/meta/maybe_const.h>
#include <di/util/non_propagating_cache.h>
#include <di/util/store_if.h>
#include <di/vocab/variant/prelude.h>

namespace di::container {
namespace detail {
    template<typename Con, typename Pattern>
    concept CompatibleJoinableContainers =
        concepts::CommonWith<meta::ContainerValue<Con>, meta::ContainerValue<Pattern>> &&
        concepts::CommonReferenceWith<meta::ContainerReference<Con>, meta::ContainerReference<Pattern>> &&
        concepts::CommonReferenceWith<meta::ContainerRValue<Con>, meta::ContainerRValue<Pattern>>;

    template<typename Con>
    concept BidirectionalCommon = concepts::BidirectionalContainer<Con> && concepts::CommonContainer<Con>;
}

template<concepts::InputContainer View, concepts::ForwardContainer Pattern>
requires(concepts::View<View> && concepts::InputContainer<meta::ContainerReference<View>> && concepts::View<Pattern> &&
         detail::CompatibleJoinableContainers<meta::ContainerReference<View>, Pattern>)
class JoinWithView : public ViewInterface<JoinWithView<View, Pattern>> {
private:
    using InnerContainer = meta::ContainerReference<View>;

    template<bool other_is_const>
    class Sentinel;

    template<bool is_const>
    class Iterator
        : public IteratorBase<
              Iterator<is_const>,
              meta::Conditional<
                  concepts::Reference<meta::ContainerReference<meta::MaybeConst<is_const, View>>> &&
                      concepts::BidirectionalContainer<meta::MaybeConst<is_const, View>> &&
                      detail::BidirectionalCommon<meta::ContainerReference<meta::MaybeConst<is_const, View>>> &&
                      detail::BidirectionalCommon<meta::MaybeConst<is_const, Pattern>>,
                  BidirectionalIteratorTag,
                  meta::Conditional<
                      concepts::Reference<meta::ContainerReference<meta::MaybeConst<is_const, View>>> &&
                          concepts::ForwardIterator<meta::ContainerIterator<meta::MaybeConst<is_const, View>>> &&
                          concepts::ForwardIterator<
                              meta::ContainerIterator<meta::ContainerReference<meta::MaybeConst<is_const, View>>>>,
                      ForwardIteratorTag, InputIteratorTag>>,
              meta::CommonType<meta::ContainerValue<meta::ContainerReference<meta::MaybeConst<is_const, View>>>,
                               meta::ContainerValue<meta::MaybeConst<is_const, Pattern>>>,
              meta::CommonType<meta::ContainerSSizeType<meta::MaybeConst<is_const, View>>,
                               meta::ContainerSSizeType<meta::ContainerReference<meta::MaybeConst<is_const, View>>>,
                               meta::ContainerSSizeType<meta::MaybeConst<is_const, Pattern>>>> {
    private:
        using Parent = meta::MaybeConst<is_const, JoinWithView>;
        using Base = meta::MaybeConst<is_const, View>;
        using InnerBase = meta::ContainerReference<Base>;
        using PatternBase = meta::MaybeConst<is_const, Pattern>;

        using OuterIter = meta::ContainerIterator<Base>;
        using InnerIter = meta::ContainerIterator<InnerBase>;
        using PatternIter = meta::ContainerIterator<PatternBase>;

        constexpr static bool ref_is_glvalue = concepts::Reference<InnerBase>;

        constexpr Iterator(Parent& parent, OuterIter outer)
            : m_parent(util::addressof(parent)), m_outer(util::move(outer)) {
            if (m_outer != container::end(parent.m_base)) {
                auto&& inner = this->update_inner(m_outer);
                m_inner.template emplace<1>(container::begin(inner));
                satisfy();
            }
        }

    public:
        Iterator()
        requires(concepts::DefaultInitializable<OuterIter>)
        = default;

        Iterator(Iterator const&) = delete;
        Iterator& operator=(Iterator const&) = delete;

        Iterator(Iterator const&)
        requires(ref_is_glvalue && concepts::ForwardIterator<OuterIter> && concepts::ForwardIterator<InnerIter>)
        = default;
        Iterator& operator=(Iterator const&)
        requires(ref_is_glvalue && concepts::ForwardIterator<OuterIter> && concepts::ForwardIterator<InnerIter>)
        = default;

        Iterator(Iterator&&) = default;
        Iterator& operator=(Iterator&&) = default;

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<meta::ContainerIterator<View>, OuterIter> &&
                 concepts::ConvertibleTo<meta::ContainerIterator<InnerContainer>, InnerIter> &&
                 concepts::ConvertibleTo<meta::ContainerIterator<Pattern>, PatternIter>)
            : m_parent(other.m_parent), m_outer(util::move(other.m_outer)), m_inner(util::move(other.m_inner)) {}

        constexpr decltype(auto) operator*() const {
            using Reference =
                meta::CommonReference<meta::IteratorReference<InnerIter>, meta::IteratorReference<PatternIter>>;
            return visit(
                [](auto& it) -> Reference {
                    return *it;
                },
                m_inner);
        }

        constexpr void advance_one() {
            visit(
                [](auto& it) {
                    ++it;
                },
                m_inner);
            satisfy();
        }

        constexpr void back_one()
        requires(ref_is_glvalue && concepts::BidirectionalContainer<Base> && detail::BidirectionalCommon<InnerBase> &&
                 detail::BidirectionalCommon<PatternBase>)
        {
            if (m_outer == container::end(m_parent->m_base)) {
                auto&& inner = *--m_outer;
                m_inner.template emplace<1>(container::end(inner));
            }

            for (;;) {
                if (m_inner.index() == 0) {
                    auto& it = util::get<0>(m_inner);
                    if (it == container::begin(m_pattern->m_pattern)) {
                        auto&& inner = *--m_outer;
                        m_inner.template emplace<1>(container::end(inner));
                    } else {
                        break;
                    }
                } else {
                    auto& it = util::get<1>(m_inner);
                    auto&& inner = *m_outer;
                    if (it == container::begin(inner)) {
                        m_inner.template emplace<0>(container::end(m_parent->m_pattern));
                    } else {
                        break;
                    }
                }
            }

            visit(
                [](auto& it) {
                    --it;
                },
                m_inner);
        }

        constexpr OuterIter const& outer() const { return m_outer; }

    private:
        template<bool other_is_const>
        friend class Iterator;

        template<bool other_is_const>
        friend class Sentinel;

        friend class JoinWithView;

        constexpr auto&& update_inner(OuterIter const& x) {
            if constexpr (ref_is_glvalue) {
                return *x;
            } else {
                return m_parent->m_inner.value.emplace_deref(x);
            }
        }

        constexpr auto&& get_inner(OuterIter const& x) {
            if constexpr (ref_is_glvalue) {
                return *x;
            } else {
                return *m_parent->m_inner.value;
            }
        }

        constexpr void satisfy() {
            for (;;) {
                if (m_inner.index() == 0) {
                    if (util::get<0>(m_inner) != container::end(m_parent->m_pattern)) {
                        break;
                    }

                    auto&& inner = this->get_inner(m_outer);
                    m_inner.template emplace<1>(container::begin(inner));
                } else {
                    auto&& inner = this->get_inner(m_outer);
                    if (util::get<1>(m_inner) != container::end(inner)) {
                        break;
                    }

                    if (++m_outer == container::end(m_parent->m_base)) {
                        if constexpr (ref_is_glvalue) {
                            m_inner.template emplace<0>();
                        }
                        break;
                    }

                    m_inner.template emplace<0>(container::begin(m_parent->m_pattern));
                }
            }
        }

        constexpr friend bool operator==(Iterator const& x, Iterator const& y)
        requires(ref_is_glvalue && concepts::EqualityComparable<OuterIter> && concepts::EqualityComparable<InnerIter>)
        {
            return x.m_outer == y.m_outer && x.m_inner == y.m_inner;
        }

        constexpr friend decltype(auto) tag_invoke(types::Tag<iterator_move>, Iterator const& x) {
            using RValue = meta::CommonReference<meta::IteratorRValue<InnerIter>, meta::IteratorRValue<PatternIter>>;
            return visit<RValue>(container::iterator_move, x.m_inner);
        }

        constexpr friend void tag_invoke(types::Tag<iterator_swap>, Iterator const& x, Iterator const& y)
        requires(concepts::IndirectlySwappable<InnerIter, PatternIter>)
        {
            visit(container::iterator_swap, x.m_inner, y.m_inner);
        }

        Parent* m_parent { nullptr };
        OuterIter m_outer {};
        Variant<PatternIter, InnerIter> m_inner;
    };

    template<bool is_const>
    class Sentinel {
    private:
        using Parent = meta::MaybeConst<is_const, JoinWithView>;
        using Base = meta::MaybeConst<is_const, View>;

        constexpr explicit Sentinel(Parent& parent) : m_base(container::end(parent.m_base)) {}

    public:
        Sentinel() = default;

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<meta::ContainerSentinel<View>, meta::ContainerSentinel<Base>>)
            : m_base(util::move(other.m_base)) {}

    private:
        template<bool other_is_const>
        friend class Iterator;

        template<bool other_is_const>
        friend class Sentinel;

        friend class JoinWithView;

        template<bool other_is_const>
        requires(concepts::SentinelFor<meta::ContainerSentinel<Base>,
                                       meta::ContainerIterator<meta::MaybeConst<other_is_const, View>>>)
        constexpr friend bool operator==(Iterator<other_is_const> const& x, Sentinel const& y) {
            return x.outer() == y.m_base;
        }

        meta::ContainerSentinel<Base> m_base;
    };

public:
    JoinWithView()
    requires(concepts::DefaultInitializable<View> && concepts::DefaultInitializable<Pattern>)
    = default;

    constexpr JoinWithView(View base, Pattern pattern) : m_base(util::move(base)), m_pattern(util::move(pattern)) {}

    template<concepts::InputContainer Con>
    requires(concepts::ConstructibleFrom<View, meta::AsView<Con>> &&
             concepts::ConstructibleFrom<Pattern, SingleView<meta::ContainerValue<InnerContainer>>>)
    constexpr JoinWithView(Con&& container, meta::ContainerValue<InnerContainer> value)
        : m_base(view::all(util::forward<Con>(container))), m_pattern(SingleView { util::move(value) }) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr auto begin() {
        constexpr bool is_const =
            concepts::SimpleView<View> && concepts::Reference<InnerContainer> && concepts::SimpleView<Pattern>;
        return Iterator<is_const>(*this, container::begin(m_base));
    }

    constexpr auto begin() const
    requires(concepts::InputContainer<View const> && concepts::ForwardContainer<Pattern const> &&
             concepts::Reference<meta::ContainerReference<View const>>)
    {
        return Iterator<true>(*this, container::begin(m_base));
    }

    constexpr auto end() {
        if constexpr (concepts::ForwardContainer<View> && concepts::Reference<InnerContainer> &&
                      concepts::ForwardContainer<InnerContainer> && concepts::CommonContainer<View> &&
                      concepts::CommonContainer<InnerContainer>) {
            return Iterator<(concepts::SimpleView<View> && concepts::SimpleView<Pattern>)>(*this,
                                                                                           container::end(m_base));
        } else {
            return Sentinel<(concepts::SimpleView<View> && concepts::SimpleView<Pattern>)>(*this);
        }
    }

    constexpr auto end() const
    requires(concepts::InputContainer<View const> && concepts::ForwardContainer<Pattern const> &&
             concepts::Reference<meta::ContainerReference<View const>>)
    {
        using ConstInnerContainer = meta::ContainerReference<View const>;
        if constexpr (concepts::ForwardContainer<View const> && concepts::ForwardContainer<ConstInnerContainer> &&
                      concepts::CommonContainer<View const> && concepts::CommonContainer<ConstInnerContainer>) {
            return Iterator<true>(*this, container::end(m_base));
        } else {
            return Sentinel<true>(*this);
        }
    }

private:
    template<bool is_const>
    friend class Iterator;

    View m_base;
    Pattern m_pattern;
    util::StoreIf<util::NonPropagatingCache<meta::RemoveCVRef<InnerContainer>>, !concepts::Reference<InnerContainer>>
        m_inner;
};

template<typename Con, typename Pattern>
JoinWithView(Con&&, Pattern&&) -> JoinWithView<meta::AsView<Con>, meta::AsView<Pattern>>;

template<concepts::InputContainer Con>
JoinWithView(Con&&, meta::ContainerValue<meta::ContainerReference<Con>>)
    -> JoinWithView<meta::AsView<Con>, SingleView<meta::ContainerValue<meta::ContainerReference<Con>>>>;
}
