#pragma once

#include <di/container/algorithm/find_if.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/iterator_move.h>
#include <di/container/iterator/iterator_swap.h>
#include <di/container/iterator/sentinel_extension.h>
#include <di/container/meta/prelude.h>
#include <di/container/types/prelude.h>
#include <di/container/view/view_interface.h>
#include <di/function/invoke.h>
#include <di/util/addressof.h>
#include <di/util/move.h>
#include <di/util/non_propagating_cache.h>
#include <di/util/rebindable_box.h>
#include <di/util/reference_wrapper.h>
#include <di/util/store_if.h>

namespace di::container {
template<concepts::InputContainer View, concepts::IndirectUnaryPredicate<meta::ContainerIterator<View>> Pred>
requires(concepts::View<View> && concepts::Object<Pred>)
class FilterView : public ViewInterface<FilterView<View, Pred>> {
private:
    using SSizeType = meta::ContainerSSizeType<View>;
    using Value = meta::ContainerValue<View>;
    using Iter = meta::ContainerIterator<View>;
    using Sent = meta::ContainerSentinel<View>;

    class Iterator
        : public IteratorBase<Iterator,
                              meta::Conditional<concepts::BidirectionalIterator<Iter>, BidirectionalIteratorTag,
                                                meta::Conditional<concepts::ForwardIterator<Iter>, ForwardIteratorTag,
                                                                  InputIteratorTag>>,
                              Value, SSizeType> {
    public:
        Iterator()
        requires(concepts::DefaultInitializable<Iter>)
        = default;

        constexpr Iterator(FilterView& parent, Iter base)
            : m_parent(util::addressof(parent)), m_base(util::move(base)) {}

        constexpr Iter const& base() const& { return m_base; }
        constexpr Iter base() && { return util::move(m_base); }

        constexpr decltype(auto) operator*() const { return *m_base; }
        constexpr Iter operator->() const
        requires(concepts::ForwardIterator<Iter> &&
                 (concepts::Pointer<Iter> || requires(Iter iter) { iter.operator->(); }) && concepts::Copyable<Iter>)
        {
            return m_base;
        }

        constexpr void advance_one() {
            m_base = container::find_if(util::move(++m_base), container::end(m_parent->m_base),
                                        util::ref(m_parent->m_predicate.value()));
        }

        constexpr void back_one()
        requires(concepts::BidirectionalIterator<Iter>)
        {
            do {
                --m_base;
            } while (!function::invoke(m_parent->m_predicate.value(), *m_base));
        }

    private:
        constexpr friend bool operator==(Iterator const& a, Iterator const& b) { return a.base() == b.base(); }

        constexpr friend meta::IteratorRValue<Iter> tag_invoke(types::Tag<iterator_move>, Iterator const& a) {
            return container::iterator_move(a);
        }

        constexpr friend void tag_invoke(Iterator const& a, Iterator const& b)
        requires(concepts::IndirectlySwappable<Iter>)
        {
            return container::iterator_swap(a, b);
        }

        FilterView* m_parent;
        Iter m_base;
    };

    class Sentinel : public SentinelExtension<Sentinel, Sent, Iterator, Iter> {};

public:
    FilterView()
    requires(concepts::DefaultInitializable<View> && concepts::DefaultInitializable<Pred>)
    = default;

    constexpr explicit FilterView(View base, Pred predicate)
        : m_base(util::move(base)), m_predicate(util::move(predicate)) {}

    constexpr View base() const&
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr View base() && { return util::move(m_base); }

    constexpr Pred const& pred() const { return m_predicate.value(); }

    constexpr Iterator begin() {
        if constexpr (!concepts::ForwardContainer<View>) {
            return Iterator(*this, container::find_if(m_base, util::ref(m_predicate.value())));
        } else {
            if (!m_cache.value.has_value()) {
                m_cache.value = container::find_if(m_base, util::ref(m_predicate.value()));
            }
            return Iterator(*this, m_cache.value.value());
        }
    }

    constexpr auto end() {
        if constexpr (concepts::CommonContainer<View>) {
            return Iterator(*this, container::end(m_base));
        } else {
            return Sentinel(container::end(*this));
        }
    }

private:
    View m_base {};
    util::RebindableBox<Pred> m_predicate;
    util::StoreIf<util::NonPropagatingCache<meta::ContainerIterator<View>>, concepts::ForwardContainer<View>> m_cache;
};

template<typename Con, typename Pred>
FilterView(Con&&, Pred) -> FilterView<meta::AsView<Con>, Pred>;
}
