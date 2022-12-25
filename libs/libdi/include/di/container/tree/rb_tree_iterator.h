#pragma once

#include <di/assert/prelude.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/tree/rb_tree_node.h>

namespace di::container {
template<typename Value>
class RBTreeIterator : public IteratorBase<RBTreeIterator<Value>, BidirectionalIteratorTag, Value, ssize_t> {
private:
    using Node = RBTreeNode<Value>;

public:
    RBTreeIterator() = default;

    constexpr explicit RBTreeIterator(Node* current, bool at_end) : m_current(current), m_at_end(at_end) {}

    constexpr Value& operator*() const {
        DI_ASSERT(!m_at_end);
        return m_current->value;
    }
    constexpr Node& node() const { return *m_current; }

    constexpr void advance_one() {
        auto* next = m_current->successor();
        if (next) {
            m_current = next;
        } else {
            m_at_end = true;
        }
    }

    constexpr void back_one() {
        if (m_at_end) {
            m_at_end = false;
            return;
        }

        m_current = m_current->predecessor();
    }

private:
    constexpr friend bool operator==(RBTreeIterator const& a, RBTreeIterator const& b) {
        return (a.m_at_end == b.m_at_end) && (a.m_current == b.m_current);
    }

    Node* m_current { nullptr };
    bool m_at_end { true };
};
}