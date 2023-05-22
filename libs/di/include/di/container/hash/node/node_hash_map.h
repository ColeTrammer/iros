#pragma once

#include <di/container/allocator/allocator.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/container/associative/map_interface.h>
#include <di/container/concepts/prelude.h>
#include <di/container/hash/default_hasher.h>
#include <di/container/hash/node/owning_node_hash_table.h>
#include <di/container/intrusive/forward_list_forward_declaration.h>
#include <di/container/vector/vector.h>
#include <di/container/view/transform.h>
#include <di/function/compare.h>
#include <di/function/equal.h>
#include <di/platform/prelude.h>
#include <di/util/deduce_create.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
namespace detail {
    template<typename Key, typename Value>
    struct NodeHashMapTag : OwningHashNodeTag<NodeHashMapTag<Key, Value>, Tuple<Key, Value>> {};
}

template<typename Key, typename Value, typename Eq = function::Equal, concepts::Hasher Hasher = DefaultHasher,
         typename Buckets = container::Vector<
             IntrusiveForwardList<HashNode<detail::NodeHashMapTag<Key, Value>>, detail::NodeHashMapTag<Key, Value>>>,
         concepts::Allocator Alloc = platform::DefaultAllocator>
class NodeHashMap
    : public OwningNodeHashTable<
          Tuple<Key, Value>, Eq, Hasher, Buckets, detail::NodeHashMapTag<Key, Value>, Alloc,
          MapInterface<
              NodeHashMap<Key, Value, Eq, Hasher, Buckets, Alloc>, Tuple<Key, Value>, Key, Value,
              HashNodeIterator<Tuple<Key, Value>, detail::NodeHashMapTag<Key, Value>>,
              container::ConstIteratorImpl<HashNodeIterator<Tuple<Key, Value>, detail::NodeHashMapTag<Key, Value>>>,
              detail::NodeHashTableMapValidForLookup<Key, Value, Eq>::template Type, false>,
          false, true> {
private:
    using Base =
        OwningNodeHashTable<Tuple<Key, Value>, Eq, Hasher, Buckets, detail::NodeHashMapTag<Key, Value>, Alloc,
                            MapInterface<NodeHashMap<Key, Value, Eq, Hasher, Buckets, Alloc>, Tuple<Key, Value>, Key,
                                         Value, HashNodeIterator<Tuple<Key, Value>, detail::NodeHashMapTag<Key, Value>>,
                                         container::ConstIteratorImpl<
                                             HashNodeIterator<Tuple<Key, Value>, detail::NodeHashMapTag<Key, Value>>>,
                                         detail::NodeHashTableMapValidForLookup<Key, Value, Eq>::template Type, false>,
                            false, true>;

public:
    NodeHashMap() = default;

    NodeHashMap(Eq, Hasher, Buckets const& comparator) : Base(Eq { comparator }) {}
};

template<concepts::InputContainer Con, concepts::TupleLike T = meta::ContainerValue<Con>>
requires(meta::TupleSize<T> == 2)
NodeHashMap<meta::TupleElement<T, 0>, meta::TupleElement<T, 1>> tag_invoke(types::Tag<util::deduce_create>,
                                                                           InPlaceTemplate<NodeHashMap>, Con&&);

template<concepts::InputContainer Con, concepts::TupleLike T = meta::ContainerValue<Con>, typename Eq>
requires(meta::TupleSize<T> == 2)
NodeHashMap<meta::TupleElement<T, 0>, meta::TupleElement<T, 1>, Eq> tag_invoke(types::Tag<util::deduce_create>,
                                                                               InPlaceTemplate<NodeHashMap>, Con&&, Eq);

template<concepts::InputContainer Con, concepts::TupleLike T = meta::ContainerValue<Con>, typename Eq, typename Hasher>
requires(meta::TupleSize<T> == 2)
NodeHashMap<meta::TupleElement<T, 0>, meta::TupleElement<T, 1>, Eq, Hasher>
tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<NodeHashMap>, Con&&, Eq, Hasher);
}
