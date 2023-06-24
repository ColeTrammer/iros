#pragma once

#include <di/container/associative/intrusive_set_interface.h>
#include <di/container/intrusive/intrusive_tag_base.h>
#include <di/container/tree/rb_tree.h>

namespace di::container {
struct DefaultIntrusiveTreeSetTag;

template<typename Tag = DefaultIntrusiveTreeSetTag>
using IntrusiveTreeSetNode = RBTreeNode<Tag>;

template<typename Self>
struct IntrusiveTreeSetTag : IntrusiveTagBase<IntrusiveTreeSetNode<Self>> {};

struct DefaultIntrusiveTreeSetTag : IntrusiveTreeSetTag<DefaultIntrusiveTreeSetTag> {};

template<typename T, typename Tag = DefaultIntrusiveTreeSetTag, concepts::StrictWeakOrder<T> Comp = function::Compare>
class IntrusiveTreeSet
    : public RBTree<T, Comp, Tag,
                    IntrusiveSetInterface<IntrusiveTreeSet<T, Tag, Comp>, T, IntrusiveTreeSetNode<Tag>,
                                          RBTreeIterator<T, Tag>, meta::ConstIterator<RBTreeIterator<T, Tag>>,
                                          detail::RBTreeValidForLookup<T, Comp>::template Type, false>,
                    false> {};

template<typename T, typename Tag = DefaultIntrusiveTreeSetTag, concepts::StrictWeakOrder<T> Comp = function::Compare>
class IntrusiveTreeMultiSet
    : public RBTree<T, Comp, Tag,
                    IntrusiveSetInterface<IntrusiveTreeMultiSet<T, Tag, Comp>, T, IntrusiveTreeSetNode<Tag>,
                                          RBTreeIterator<T, Tag>, meta::ConstIterator<RBTreeIterator<T, Tag>>,
                                          detail::RBTreeValidForLookup<T, Comp>::template Type, true>,
                    true> {};
}

namespace di {
using container::IntrusiveTreeMultiSet;
using container::IntrusiveTreeSet;
using container::IntrusiveTreeSetNode;
using container::IntrusiveTreeSetTag;
}
