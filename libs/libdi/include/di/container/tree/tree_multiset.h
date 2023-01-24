#pragma once

#include <di/container/allocator/prelude.h>
#include <di/container/associative/set_interface.h>
#include <di/container/concepts/prelude.h>
#include <di/container/tree/rb_tree.h>
#include <di/container/view/transform.h>
#include <di/function/compare.h>
#include <di/platform/prelude.h>
#include <di/util/deduce_create.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
template<typename Value, concepts::StrictWeakOrder<Value> Comp = function::Compare,
         concepts::AllocatorOf<RBTreeNode<Value>> Alloc = DefaultAllocator<RBTreeNode<Value>>>
class TreeMultiSet
    : public RBTree<Value, Comp, Alloc,
                    SetInterface<TreeMultiSet<Value, Comp, Alloc>, Value, RBTreeIterator<Value>, meta::ConstIterator<RBTreeIterator<Value>>,
                                 detail::RBTreeValidForLookup<Value, Comp>::template Type, true>,
                    true> {
private:
    using Base =
        RBTree<Value, Comp, Alloc,
               SetInterface<TreeMultiSet<Value, Comp, Alloc>, Value, RBTreeIterator<Value>, meta::ConstIterator<RBTreeIterator<Value>>,
                            detail::RBTreeValidForLookup<Value, Comp>::template Type, true>,
               true>;

public:
    using Base::Base;
};

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
TreeMultiSet<T> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<TreeMultiSet>, Con&&);

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>, concepts::StrictWeakOrder<T> Comp>
TreeMultiSet<T, Comp> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<TreeMultiSet>, Con&&, Comp);
}