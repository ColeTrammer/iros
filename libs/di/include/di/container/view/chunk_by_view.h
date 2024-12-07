#pragma once

#include "di/container/algorithm/adjacent_find.h"
#include "di/container/concepts/prelude.h"
#include "di/container/iterator/default_sentinel.h"
#include "di/container/iterator/iterator_base.h"
#include "di/container/iterator/iterator_move.h"
#include "di/container/iterator/iterator_swap.h"
#include "di/container/iterator/sentinel_extension.h"
#include "di/container/meta/prelude.h"
#include "di/container/types/prelude.h"
#include "di/container/view/reverse.h"
#include "di/container/view/view_interface.h"
#include "di/function/flip.h"
#include "di/function/invoke.h"
#include "di/function/not_fn.h"
#include "di/util/addressof.h"
#include "di/util/move.h"
#include "di/util/non_propagating_cache.h"
#include "di/util/rebindable_box.h"
#include "di/util/reference_wrapper.h"
#include "di/util/store_if.h"

namespace di::container {
template<concepts::ForwardContainer View, concepts::IndirectBinaryPredicate<meta::ContainerIterator<View>> Pred>
requires(concepts::View<View> && concepts::Object<Pred>)
class ChunkByView : public ViewInterface<ChunkByView<View, Pred>> {
private:
    using SSizeType = meta::ContainerSSizeType<View>;
    using Iter = meta::ContainerIterator<View>;
    using Value = meta::Reconstructed<View, Iter>;

    class Iterator
        : public IteratorBase<
              Iterator,
              meta::Conditional<concepts::BidirectionalIterator<Iter>, BidirectionalIteratorTag, ForwardIteratorTag>,
              Value, SSizeType> {
    private:
        friend class ChunkByView;

        constexpr Iterator(ChunkByView& parent, Iter current, Iter next)
            : m_parent(util::addressof(parent)), m_current(util::move(current)), m_next(util::move(next)) {}

    public:
        Iterator()
        requires(concepts::DefaultInitializable<Iter>)
        = default;

        constexpr auto operator*() const -> decltype(auto) {
            return container::reconstruct(in_place_type<View>, m_current, m_next);
        }

        constexpr void advance_one() {
            m_current = m_next;
            m_next = m_parent->find_next(m_current);
        }

        constexpr void back_one()
        requires(concepts::BidirectionalIterator<Iter>)
        {
            m_next = m_current;
            m_current = m_parent->find_prev(m_next);
        }

    private:
        constexpr friend auto operator==(Iterator const& a, Iterator const& b) -> bool {
            return a.m_current == b.m_current;
        }
        constexpr friend auto operator==(Iterator const& a, DefaultSentinel) -> bool { return a.m_current == a.m_next; }

        ChunkByView* m_parent { nullptr };
        Iter m_current {};
        Iter m_next {};
    };

public:
    ChunkByView()
    requires(concepts::DefaultInitializable<View> && concepts::DefaultInitializable<Pred>)
    = default;

    constexpr explicit ChunkByView(View base, Pred predicate)
        : m_base(util::move(base)), m_predicate(util::move(predicate)) {}

    constexpr auto base() const& -> View
    requires(concepts::CopyConstructible<View>)
    {
        return m_base;
    }
    constexpr auto base() && -> View { return util::move(m_base); }

    constexpr auto pred() const -> Pred const& { return m_predicate.value(); }

    constexpr auto begin() -> Iterator {
        if (!m_cache) {
            m_cache = Iterator(*this, container::begin(m_base), find_next(container::begin(m_base)));
        }
        return *m_cache;
    }

    constexpr auto end() {
        if constexpr (concepts::CommonContainer<View>) {
            return Iterator(*this, container::end(m_base), container::end(m_base));
        } else {
            return default_sentinel;
        }
    }

private:
    constexpr auto find_next(Iter current) -> Iter {
        return container::next(
            container::adjacent_find(current, container::end(m_base), function::not_fn(util::ref(m_predicate.value()))),
            1, container::end(m_base));
    }

    constexpr auto find_prev(Iter current) -> Iter
    requires(concepts::BidirectionalContainer<View>)
    {
        auto reversed = container::View(container::begin(m_base), current) | view::reverse;
        return container::prev(
            container::adjacent_find(reversed, function::not_fn(function::flip(util::ref(m_predicate.value())))).base(),
            1, container::begin(m_base));
    }

    View m_base {};
    util::RebindableBox<Pred> m_predicate;
    util::NonPropagatingCache<Iterator> m_cache;
};

template<typename Con, typename Pred>
ChunkByView(Con&&, Pred) -> ChunkByView<meta::AsView<Con>, Pred>;
}
