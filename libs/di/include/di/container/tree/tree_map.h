#pragma once

#include <di/container/allocator/allocator.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/container/associative/map_interface.h>
#include <di/container/concepts/prelude.h>
#include <di/container/tree/owning_rb_tree.h>
#include <di/container/tree/rb_tree.h>
#include <di/container/view/transform.h>
#include <di/function/compare.h>
#include <di/platform/prelude.h>
#include <di/util/deduce_create.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
namespace detail {
    template<typename Comp, typename Key>
    struct TreeMapCompAdapter {
        [[no_unique_address]] Comp comp {};

        template<typename U, typename V1, typename V2>
        requires(concepts::StrictWeakOrder<Comp const&, Key, U>)
        constexpr auto operator()(Tuple<Key, V1> const& a, Tuple<U, V2> const& b) const {
            return function::invoke(comp, util::get<0>(a), util::get<0>(b));
        }

        template<typename U, typename V1, typename V2>
        requires(!concepts::SameAs<U, Key> && concepts::StrictWeakOrder<Comp const&, Key, U>)
        constexpr auto operator()(Tuple<U, V1> const& a, Tuple<Key, V2> const& b) const {
            return function::invoke(comp, util::get<0>(a), util::get<0>(b));
        }

        template<typename U, typename V1>
        requires(concepts::StrictWeakOrder<Comp const&, Key, U>)
        constexpr auto operator()(Tuple<Key, V1> const& a, U const& b) const {
            return function::invoke(comp, util::get<0>(a), b);
        }

        template<typename U, typename V2>
        requires(concepts::StrictWeakOrder<Comp const&, Key, U>)
        constexpr auto operator()(U const& a, Tuple<Key, V2> const& b) const {
            return function::invoke(comp, a, util::get<0>(b));
        }

        template<typename U>
        requires(concepts::StrictWeakOrder<Comp const&, Key, U>)
        constexpr auto operator()(Key const& a, U const& b) const {
            return function::invoke(comp, a, b);
        }

        template<typename U>
        requires(!concepts::SameAs<Key, U> && concepts::StrictWeakOrder<Comp const&, Key, U>)
        constexpr auto operator()(U const& a, Key const& b) const {
            return function::invoke(comp, a, b);
        }

        template<typename T, typename U>
        requires(concepts::StrictWeakOrder<Comp const&, T, U>)
        constexpr auto operator()(T const& a, U const& b) const {
            return function::invoke(comp, a, b);
        }
    };

    template<typename Key, typename Value>
    struct TreeMapTag : OwningRBTreeTag<TreeMapTag<Key, Value>, Tuple<Key, Value>> {};
}

template<typename Key, typename Value, concepts::StrictWeakOrder<Key> Comp = function::Compare,
         concepts::Allocator Alloc = DefaultAllocator>
class TreeMap
    : public OwningRBTree<
          Tuple<Key, Value>, detail::TreeMapCompAdapter<Comp, Key>, detail::TreeMapTag<Key, Value>, Alloc,
          MapInterface<
              TreeMap<Key, Value, Comp, Alloc>, Tuple<Key, Value>, Key, Value,
              RBTreeIterator<Tuple<Key, Value>, detail::TreeMapTag<Key, Value>>,
              container::ConstIteratorImpl<RBTreeIterator<Tuple<Key, Value>, detail::TreeMapTag<Key, Value>>>,
              detail::RBTreeValidForLookup<Tuple<Key, Value>, detail::TreeMapCompAdapter<Comp, Key>>::template Type,
              false>,
          false> {
private:
    using Base = OwningRBTree<
        Tuple<Key, Value>, detail::TreeMapCompAdapter<Comp, Key>, detail::TreeMapTag<Key, Value>, Alloc,
        MapInterface<
            TreeMap<Key, Value, Comp, Alloc>, Tuple<Key, Value>, Key, Value,
            RBTreeIterator<Tuple<Key, Value>, detail::TreeMapTag<Key, Value>>,
            container::ConstIteratorImpl<RBTreeIterator<Tuple<Key, Value>, detail::TreeMapTag<Key, Value>>>,
            detail::RBTreeValidForLookup<Tuple<Key, Value>, detail::TreeMapCompAdapter<Comp, Key>>::template Type,
            false>,
        false>;

public:
    TreeMap() = default;

    TreeMap(Comp const& comparator) : Base(detail::TreeMapCompAdapter<Comp, Key> { comparator }) {}
};

template<concepts::InputContainer Con, concepts::TupleLike T = meta::ContainerValue<Con>>
requires(meta::TupleSize<T> == 2)
auto tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<TreeMap>, Con&&)
    -> TreeMap<meta::TupleElement<T, 0>, meta::TupleElement<T, 1>>;

template<concepts::InputContainer Con, concepts::TupleLike T = meta::ContainerValue<Con>,
         concepts::StrictWeakOrder<T> Comp>
requires(meta::TupleSize<T> == 2)
auto tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<TreeMap>, Con&&, Comp)
    -> TreeMap<meta::TupleElement<T, 0>, meta::TupleElement<T, 1>, Comp>;
}

namespace di {
using container::TreeMap;
}
