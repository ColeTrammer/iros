#pragma once

#include <di/container/tree/rb_tree.h>

namespace di::container {
template<typename Self, typename T>
struct OwningRBTreeTag;

template<typename T, typename Tag>
struct OwningRBTreeNode : RBTreeNode<Tag> {
public:
    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr OwningRBTreeNode(InPlace, Args&&... args) : m_value(util::forward<Args>(args)...) {}

    constexpr T& value() { return m_value; }

private:
    T m_value;
};

template<typename Self, typename T>
struct OwningRBTreeTag : IntrusiveTagBase<OwningRBTreeNode<T, Self>> {
    using Node = OwningRBTreeNode<T, Self>;

    constexpr static bool is_sized(InPlaceType<T>) { return true; }
    constexpr static T& down_cast(InPlaceType<T>, Node& node) { return node.value(); }

    constexpr static void did_remove(auto& self, auto& node) {
        util::destroy_at(util::addressof(node));
        self.allocator().deallocate(util::addressof(node), 1);
    }
};

template<typename Value, concepts::StrictWeakOrder<Value> Comp, typename Tag,
         concepts::AllocatorOf<OwningRBTreeNode<Value, Tag>> Alloc, typename Interface, bool is_multi>
class OwningRBTree
    : public RBTree<Value, Comp, Tag, Interface, is_multi, OwningRBTree<Value, Comp, Tag, Alloc, Interface, is_multi>> {
private:
    using Base =
        RBTree<Value, Comp, Tag, Interface, is_multi, OwningRBTree<Value, Comp, Tag, Alloc, Interface, is_multi>>;

    using Node = RBTreeNode<Tag>;
    using Iterator = RBTreeIterator<Value, Tag>;
    using ConstIterator = meta::ConstIterator<Iterator>;

public:
    using Base::Base;

    constexpr Alloc allocator() const { return Alloc(); }

    template<typename U, concepts::Invocable F>
    requires(concepts::StrictWeakOrder<Comp&, Value, U> && concepts::MaybeFallible<meta::InvokeResult<F>, Value>)
    constexpr auto insert_with_factory(U&& needle, F&& factory) {
        auto position = this->insert_position(needle);
        if constexpr (!is_multi) {
            if (position.parent && this->compare(this->node_value(*position.parent), needle) == 0) {
                return Tuple(Iterator(position.parent, false), false);
            }
        }

        auto* node = this->create_node(function::invoke(util::forward<F>(factory)));
        this->insert_node(position, *node);
        if constexpr (!is_multi) {
            return Tuple(Iterator(node, false), true);
        } else {
            return Iterator(node, false);
        }
    }

    template<typename U, concepts::Invocable F>
    requires(concepts::StrictWeakOrder<Comp&, Value, U> && concepts::MaybeFallible<meta::InvokeResult<F>, Value>)
    constexpr auto insert_with_factory(ConstIterator, U&& needle, F&& factory) {
        auto position = this->insert_position(needle);
        if constexpr (!is_multi) {
            if (position.parent && this->compare(this->node_value(*position.parent), needle) == 0) {
                return Iterator(position.parent, false);
            }
        }

        auto* node = this->create_node(function::invoke(util::forward<F>(factory)));
        this->insert_node(position, *node);
        return Iterator(node, false);
    }

private:
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr Node* create_node(Args&&... args) {
        auto [pointer, allocated_nodes] = Alloc().allocate(1);
        (void) allocated_nodes;

        util::construct_at(pointer, in_place, util::forward<Args>(args)...);
        return pointer;
    }
};
}
