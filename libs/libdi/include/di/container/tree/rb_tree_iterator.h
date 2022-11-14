#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/tree/rb_tree_node.h>

namespace di::container {
template<typename Value>
class RBTreeIterator : public IteratorBase<RBTreeIterator<Value>, Value, ssize_t> {
private:
    using Node = RBTreeNode<Value>;

public:
    RBTreeIterator() = default;

    constexpr explicit RBTreeIterator(Node* current) : m_current(current) {}

    constexpr Value& operator*() const { return m_current->value; }
    constexpr Node& node() const { return *m_current; }

    constexpr void advance_one() {
        if (m_current->right != nullptr) {
            // Find the minimum value of the right sub-tree.
            m_current = m_current->right;
            while (m_current && m_current->left) {
                m_current = m_current->left;
            }
            return;
        }

        auto* child = m_current;
        auto* parent = child->parent;
        while (parent != nullptr && child == parent->right) {
            child = parent;
            parent = parent->parent;
        }
        m_current = parent;
    }

private:
    constexpr friend bool operator==(RBTreeIterator const& a, RBTreeIterator const& b) { return a.m_current == b.m_current; }

    constexpr friend types::ForwardIteratorTag tag_invoke(types::Tag<iterator_category>, InPlaceType<RBTreeIterator>) {
        return types::ForwardIteratorTag {};
    }

    Node* m_current { nullptr };
};
}