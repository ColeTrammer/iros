#pragma once

#include <di/container/allocator/allocator.h>
#include <di/container/allocator/fallible_allocator.h>
#include <di/container/allocator/infallible_allocator.h>
#include <di/container/associative/set_interface.h>
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
    template<typename Value>
    struct TreeSetTag : OwningRBTreeTag<TreeSetTag<Value>, Value> {};
}

template<typename Value, concepts::StrictWeakOrder<Value> Comp = function::Compare,
         concepts::Allocator Alloc = DefaultAllocator>
class TreeSet
    : public OwningRBTree<
          Value, Comp, detail::TreeSetTag<Value>, Alloc,
          SetInterface<TreeSet<Value, Comp, Alloc>, Value, RBTreeIterator<Value, detail::TreeSetTag<Value>>,
                       container::ConstIteratorImpl<RBTreeIterator<Value, detail::TreeSetTag<Value>>>,
                       detail::RBTreeValidForLookup<Value, Comp>::template Type, false>,
          false> {
private:
    using Base =
        OwningRBTree<Value, Comp, detail::TreeSetTag<Value>, Alloc,
                     SetInterface<TreeSet<Value, Comp, Alloc>, Value, RBTreeIterator<Value, detail::TreeSetTag<Value>>,
                                  container::ConstIteratorImpl<RBTreeIterator<Value, detail::TreeSetTag<Value>>>,
                                  detail::RBTreeValidForLookup<Value, Comp>::template Type, false>,
                     false>;

public:
    using Base::Base;
};

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>>
auto tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<TreeSet>, Con&&) -> TreeSet<T>;

template<concepts::InputContainer Con, typename T = meta::ContainerValue<Con>, concepts::StrictWeakOrder<T> Comp>
auto tag_invoke(types::Tag<util::deduce_create>, InPlaceTemplate<TreeSet>, Con&&, Comp) -> TreeSet<T, Comp>;
}

namespace di {
using container::TreeSet;
}
