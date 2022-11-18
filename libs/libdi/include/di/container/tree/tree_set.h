#pragma once

#include <di/container/allocator/prelude.h>
#include <di/container/concepts/prelude.h>
#include <di/container/tree/rb_tree.h>
#include <di/container/view/transform.h>
#include <di/function/compare.h>
#include <di/util/deduce_create.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
template<typename Value, concepts::StrictWeakOrder<Value> Comp = function::Compare,
         concepts::AllocatorOf<RBTreeNode<Value>> Alloc = Allocator<RBTreeNode<Value>>>
class TreeSet : public RBTree<Value, Comp, Alloc> {
private:
    using Base = RBTree<Value, Comp, Alloc>;
    using Node = RBTreeNode<Value>;

public:
    using Base::Base;

private:
};

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
TreeSet<T> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<TreeSet>, Con&&);

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>, concepts::StrictWeakOrder<T> Comp>
TreeSet<T, Comp> tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<TreeSet>, Con&&, Comp);
}