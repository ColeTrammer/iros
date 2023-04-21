#pragma once

#include <di/container/associative/intrusive_set_interface.h>
#include <di/container/hash/default_hasher.h>
#include <di/container/hash/node/hash_node.h>
#include <di/container/hash/node/node_hash_table.h>
#include <di/container/intrusive/forward_list.h>
#include <di/container/intrusive/intrusive_tag_base.h>
#include <di/container/vector/mutable_vector.h>
#include <di/container/vector/vector.h>

namespace di::container {
template<typename Tag>
using IntrusiveHashSetNode = HashNode<Tag>;

template<typename Self>
struct IntrusiveHashSetTag : IntrusiveForwardListTag<IntrusiveHashSetNode<Self>> {};

struct DefaultIntrusiveHashSetTag : IntrusiveHashSetTag<DefaultIntrusiveHashSetTag> {};

template<typename T, typename Tag = DefaultIntrusiveHashSetTag,
         concepts::Predicate<T const&, T const&> Eq = function::Equal, concepts::Hasher Hasher = DefaultHasher,
         concepts::detail::MutableVector Buckets = Vector<IntrusiveForwardList<T, Tag>>>
class IntrusiveHashSet
    : public NodeHashTable<
          T, Eq, Hasher, Buckets, Tag,
          IntrusiveSetInterface<IntrusiveHashSet<T, Tag, Eq, Hasher>, T, IntrusiveHashSetNode<Tag>,
                                HashNodeIterator<T, Tag>, meta::ConstIterator<HashNodeIterator<T, Tag>>,
                                detail::NodeHashTableValidForLookup<T, Eq>::template Type, false>,
          false> {};

template<typename T, typename Tag = DefaultIntrusiveHashSetTag,
         concepts::Predicate<T const&, T const&> Eq = function::Equal, concepts::Hasher Hasher = DefaultHasher,
         concepts::detail::MutableVector Buckets = Vector<IntrusiveForwardList<T, Tag>>>
class IntrusiveHashMultiSet
    : public NodeHashTable<
          T, Eq, Hasher, Buckets, Tag,
          IntrusiveSetInterface<IntrusiveHashSet<T, Tag, Eq, Hasher>, T, IntrusiveHashSetNode<Tag>,
                                HashNodeIterator<T, Tag>, meta::ConstIterator<HashNodeIterator<T, Tag>>,
                                detail::NodeHashTableValidForLookup<T, Eq>::template Type, true>,
          true> {};
}
