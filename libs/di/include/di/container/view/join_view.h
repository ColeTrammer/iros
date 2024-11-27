#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/interface/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/meta/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/meta/util.h>
#include <di/util/non_propagating_cache.h>
#include <di/util/store_if.h>

namespace di::container {
template<concepts::InputContainer View>
requires(concepts::View<View> && concepts::InputContainer<meta::ContainerReference<View>>)
class JoinView : public ViewInterface<JoinView<View>> {
private:
    template<bool is_const>
    using Base = meta::MaybeConst<is_const, View>;

    template<bool is_const>
    using Value = meta::ContainerValue<meta::ContainerReference<View>>;

    template<bool is_const>
    using Sent = meta::ContainerSentinel<Base<is_const>>;

    template<bool is_const>
    using SSizeType = meta::CommonType<meta::ContainerSSizeType<Base<is_const>>,
                                       meta::ContainerSSizeType<meta::ContainerReference<Base<is_const>>>>;

    template<typename Con>
    constexpr static bool IsCommon =
        concepts::ForwardContainer<Con> && concepts::Reference<meta::ContainerReference<Con>> &&
        concepts::ForwardContainer<meta::ContainerReference<Con>> && concepts::CommonContainer<Con> &&
        concepts::CommonContainer<meta::ContainerReference<Con>>;

    template<typename Con>
    constexpr static bool IsBidirectional =
        concepts::Reference<meta::ContainerReference<Con>> && concepts::BidirectionalContainer<Con> &&
        concepts::BidirectionalContainer<meta::ContainerReference<Con>>;

    template<typename Con>
    constexpr static bool IsForward =
        concepts::Reference<meta::ContainerReference<Con>> && concepts::ForwardContainer<Con> &&
        concepts::ForwardContainer<meta::ContainerReference<Con>>;

    template<bool is_const>
    class Iterator
        : public IteratorBase<
              Iterator<is_const>,
              meta::Conditional<IsBidirectional<Base<is_const>>, BidirectionalIteratorTag,
                                meta::Conditional<IsForward<Base<is_const>>, ForwardIteratorTag, InputIteratorTag>>,
              Value<is_const>, SSizeType<is_const>> {
    private:
        using Parent = meta::MaybeConst<is_const, JoinView>;
        using Outer = meta::ContainerIterator<Base<is_const>>;
        using Inner = meta::ContainerIterator<meta::ContainerReference<Base<is_const>>>;

        constexpr static bool ref_is_glvalue = concepts::Reference<meta::IteratorReference<Outer>>;

    public:
        Iterator()
        requires(concepts::DefaultInitializable<Outer> && concepts::DefaultInitializable<Inner>)
        = default;

        Iterator(Iterator const&) = delete;
        auto operator=(Iterator const&) -> Iterator& = delete;

        Iterator(Iterator const&)
        requires(IsForward<Base<is_const>>)
        = default;
        auto operator=(Iterator const&) -> Iterator& requires(IsForward<Base<is_const>>) = default;

        Iterator(Iterator&&) = default;
        auto operator=(Iterator&&) -> Iterator& = default;

        constexpr Iterator(Parent& parent, Outer outer)
            : m_parent(util::addressof(parent)), m_outer(util::move(outer)) {
            this->satisfy();
        }

        constexpr Iterator(Iterator<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<meta::ContainerIterator<View>, Outer> &&
                 concepts::ConvertibleTo<meta::ContainerIterator<meta::ContainerReference<View>>, Inner>)
            : m_parent(other.m_parent), m_outer(util::move(other.m_outer)), m_inner(util::move(other.m_outer)) {}

        constexpr auto operator*() const -> decltype(auto) { return *m_inner; }

        constexpr auto operator->() const -> Inner
        requires(requires(Inner const i) { i.operator->(); })
        {
            return m_inner;
        }

        constexpr auto outer() const -> Outer const& { return m_outer; }

        constexpr void advance_one() {
            auto&& inner_container = [&]() -> auto&& {
                if constexpr (ref_is_glvalue) {
                    return *m_outer;
                } else {
                    return *m_parent->m_inner.value;
                }
            }();

            if (++m_inner == container::end(inner_container)) {
                ++m_outer;
                satisfy();
            }
        }

        constexpr void back_one()
        requires(IsBidirectional<Base<is_const>>)
        {
            if (m_outer == container::end(m_parent->m_base)) {
                m_inner = container::end(*--m_outer);
            }
            while (m_inner == container::begin(*m_outer)) {
                m_inner = container::end(*--m_outer);
            }
            --m_inner;
        }

    private:
        template<bool other_is_const>
        friend class Iterator;

        constexpr void satisfy() {
            auto update_inner = [&](meta::ContainerIterator<Base<is_const>> x) -> auto&& {
                if constexpr (ref_is_glvalue) {
                    return *x;
                } else {
                    return m_parent->m_inner.value.emplace_deref(x);
                }
            };

            for (; m_outer != container::end(m_parent->m_base); ++m_outer) {
                auto&& inner = update_inner(m_outer);
                m_inner = container::begin(inner);
                if (m_inner != container::end(inner)) {
                    return;
                }
            }

            if constexpr (ref_is_glvalue) {
                m_inner = Inner();
            }
        }

        constexpr friend auto operator==(Iterator const& a, Iterator const& b) -> bool
        requires(ref_is_glvalue && concepts::EqualityComparable<Inner> && concepts::EqualityComparable<Outer>)
        {
            return a.m_outer == b.m_outer && a.m_inner == b.m_inner;
        }

        constexpr friend auto tag_invoke(types::Tag<iterator_move>, Iterator const& a) -> decltype(auto) {
            return container::iterator_move(a.m_inner);
        }

        constexpr friend void tag_invoke(types::Tag<iterator_swap>, Iterator const& a, Iterator const& b)
        requires(concepts::IndirectlySwappable<Inner>)
        {
            return container::iterator_swap(a.m_inner, b.m_inner);
        }

        Parent* m_parent { nullptr };
        Outer m_outer {};
        Inner m_inner {};
    };

    template<bool is_const>
    class Sentinel {
    private:
        using Parent = meta::MaybeConst<is_const, JoinView>;

    public:
        Sentinel() = default;

        constexpr explicit Sentinel(Parent& parent) : m_base(container::end(parent.m_base)) {}

        constexpr Sentinel(Sentinel<!is_const> other)
        requires(is_const && concepts::ConvertibleTo<Sent<false>, Sent<true>>)
            : m_base(other.base()) {}

        constexpr auto base() const { return m_base; }

    private:
        constexpr friend auto operator==(Iterator<is_const> const& a, Sentinel const& b) -> bool {
            return a.outer() == b.m_base;
        }

        Sent<is_const> m_base;
    };

public:
    JoinView()
    requires(concepts::DefaultInitializable<View>)
    = default;

    constexpr explicit JoinView(View base) : m_base(util::move(base)) {}

    constexpr auto base() const& -> View
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr auto base() && -> View { return util::move(m_base); }

    constexpr auto begin()
    requires(!concepts::SimpleView<View> || !concepts::Reference<meta::ContainerReference<View>>)
    {
        return Iterator<false>(*this, container::begin(m_base));
    }

    constexpr auto begin() const
    requires(concepts::InputContainer<View const> && concepts::Reference<meta::ContainerReference<View const>>)
    {
        return Iterator<true>(*this, container::begin(m_base));
    }

    constexpr auto end() {
        if constexpr (IsCommon<View>) {
            return Iterator<concepts::SimpleView<View>>(*this, container::end(m_base));
        } else {
            return Sentinel<concepts::SimpleView<View>>(*this);
        }
    }

    constexpr auto end() const
    requires(concepts::InputContainer<View const> && concepts::Reference<meta::ContainerReference<View const>>)
    {
        if constexpr (IsCommon<View const>) {
            return Iterator<true>(*this, container::end(m_base));
        } else {
            return Sentinel<true>(*this);
        }
    }

private:
    template<bool is_const>
    friend class Iterator;

    View m_base;
    util::StoreIf<util::NonPropagatingCache<meta::RemoveCVRef<meta::ContainerReference<View>>>,
                  !concepts::Reference<meta::ContainerReference<View>>>
        m_inner;
};

template<typename Con>
explicit JoinView(Con&&) -> JoinView<meta::AsView<Con>>;
}
