#pragma once

#include <di/container/concepts/prelude.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/linked/linked_list_node.h>
#include <di/container/meta/prelude.h>

namespace di::container {
template<typename Value>
class LinkedListIterator : public IteratorBase<LinkedListIterator<Value>, Value, ssize_t> {
public:
    LinkedListIterator() = default;

    constexpr explicit LinkedListIterator(LinkedListNode* node) : m_node(node) {}

    constexpr Value& operator*() const { return concrete_node().value; }
    constexpr Value* operator->() const { return util::address_of(**this); }

    constexpr void advance_one() { m_node = m_node->next; }
    constexpr void back_one() { m_node = m_node->prev; }

    constexpr decltype(auto) concrete_node() const { return static_cast<ConcreteLinkedListNode<Value>&>(*m_node); }
    constexpr LinkedListNode* node() const { return m_node; }

private:
    constexpr friend bool operator==(LinkedListIterator const& a, LinkedListIterator const& b) { return a.m_node == b.m_node; }

    constexpr friend auto tag_invoke(types::Tag<iterator_category>, InPlaceType<LinkedListIterator>) {
        return types::BidirectionalIteratorTag {};
    }

    LinkedListNode* m_node { nullptr };
};
}