#pragma once

#include <di/assert/prelude.h>
#include <di/container/allocator/prelude.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/distance.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/const_iterator.h>
#include <di/container/tree/rb_tree_iterator.h>
#include <di/container/tree/rb_tree_node.h>
#include <di/function/compare.h>
#include <di/util/exchange.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
// The book Introduction to Algorithms, Third Edition (by Thomas H. Cormen, et al.)
// was heavily referenced in this class's implementation of a Red-Black tree.
// See https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/.
template<typename Value, concepts::StrictWeakOrder<Value> Comp = function::Compare,
         concepts::AllocatorOf<RBTreeNode<Value>> Alloc = Allocator<RBTreeNode<Value>>>
class RBTree {
private:
    using Node = RBTreeNode<Value>;
    using Iterator = RBTreeIterator<Value>;
    using ConstIterator = meta::ConstIterator<Iterator>;

public:
    RBTree() = default;
    RBTree(RBTree const&) = delete;
    RBTree& operator=(RBTree const&) = delete;

    constexpr explicit RBTree(Comp comparator) : m_comparator(comparator) {}

    constexpr RBTree(RBTree&& other)
        : m_root(util::exchange(other.m_root, nullptr))
        , m_minimum(util::exchange(other.m_minimum, nullptr))
        , m_maximum(util::exchange(other.m_maximum, nullptr))
        , m_size(util::exchange(other.m_size, 0))
        , m_comparator(other.m_comparator) {}

    constexpr RBTree& operator=(RBTree&& other) {
        this->clear();
        m_root = util::exchange(other.m_root, nullptr);
        m_minimum = util::exchange(other.m_minimum, nullptr);
        m_maximum = util::exchange(other.m_maximum, nullptr);
        m_size = util::exchange(other.m_size, 0);
        m_comparator = other.m_comparator;
        return *this;
    }

    constexpr ~RBTree() { clear(); }

    constexpr bool empty() const { return size() == 0; }
    constexpr size_t size() const { return m_size; }

    constexpr Iterator begin() { return begin_impl(); }
    constexpr ConstIterator begin() const { return begin_impl(); }
    constexpr Iterator end() { return end_impl(); }
    constexpr ConstIterator end() const { return end_impl(); }

    constexpr Optional<Value&> front() {
        return lift_bool(m_minimum) % [&] {
            return util::ref(m_minimum->value);
        };
    }
    constexpr Optional<Value const&> front() const {
        return lift_bool(m_minimum) % [&] {
            return util::cref(m_minimum->value);
        };
    }

    constexpr Optional<Value&> back() {
        return lift_bool(m_maximum) % [&] {
            return util::ref(m_maximum->value);
        };
    }
    constexpr Optional<Value const&> back() const {
        return lift_bool(m_maximum) % [&] {
            return util::cref(m_maximum->value);
        };
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr auto at(U const& needle) {
        auto it = find_impl(needle);
        return lift_bool(it != end()) % [&] {
            return util::ref(*it);
        };
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr auto at(U const& needle) const {
        auto it = find_impl(needle);
        return lift_bool(it != end()) % [&] {
            return util::cref(*it);
        };
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr bool contains(U const& needle) const {
        return !!at(needle);
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr Iterator find(U const& needle) {
        return find_impl(needle);
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr size_t count(U const& needle) const {
        return container::distance(equal_range(needle));
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr ConstIterator find(U const& needle) const {
        return find_impl(needle);
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr Iterator lower_bound(U const& needle) {
        return lower_bound_impl(needle);
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr ConstIterator lower_bound(U const& needle) const {
        return lower_bound_impl(needle);
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr Iterator upper_bound(U const& needle) {
        return upper_bound_impl(needle);
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr Iterator upper_bound(U const& needle) const {
        return upper_bound_impl(needle);
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr View<Iterator> equal_range(U const& needle) {
        return equal_range_impl(needle);
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr View<ConstIterator> equal_range(U const& needle) const {
        return equal_range_impl(needle);
    }

    constexpr void clear() {
        auto end = this->end();
        for (auto it = this->begin(); it != end;) {
            auto& to_delete = (it++).node();
            erase_node(to_delete);
            destroy_node(to_delete);
        }

        m_root = m_minimum = m_maximum = nullptr;
        m_size = 0;
    }

    constexpr void insert(Value const& value)
    requires(concepts::CopyConstructible<Value>)
    {
        insert_node(*create_node(value));
    }
    constexpr void insert(Value&& value) { insert_node(*create_node(util::move(value))); }

    constexpr Iterator erase(ConstIterator position) {
        DI_ASSERT(position != end());

        auto result = container::next(position).base();
        auto& node = position.base().node();
        erase_node(node);
        destroy_node(node);
        return result;
    }

    constexpr Iterator erase(ConstIterator start, ConstIterator end) {
        for (auto it = start; it != end;) {
            erase(it++);
        }
        return end.base();
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr size_t erase(U const& needle) {
        auto [start, end] = equal_range(needle);
        size_t result = 0;
        for (auto it = start; it != end; ++result) {
            erase(it++);
        }
        return result;
    }

private:
    // Compute the color of a node, defaulting to Black.
    constexpr Node::Color node_color(Node* node) const {
        if (!node) {
            return Node::Color::Black;
        }
        return node->color;
    }

    // Swaps the passed node with its right child in the tree.
    constexpr void rotate_left(Node& x) {
        DI_ASSERT(x.right);

        auto& y = *x.right;
        x.right = y.left;
        if (y.left != nullptr) {
            y.left->parent = &x;
        }
        y.parent = x.parent;
        if (x.parent == nullptr) {
            m_root = &y;
        } else if (x.is_left_child()) {
            x.parent->left = &y;
        } else {
            x.parent->right = &y;
        }
        y.left = &x;
        x.parent = &y;
    }

    // Swaps the passed node with its left child in the tree.
    constexpr void rotate_right(Node& y) {
        DI_ASSERT(y.left);

        auto& x = *y.left;
        y.left = x.right;
        if (y.left != nullptr) {
            y.left->parent = &y;
        }
        x.parent = y.parent;
        if (y.parent == nullptr) {
            m_root = &x;
        } else if (y.is_left_child()) {
            y.parent->left = &x;
        } else {
            y.parent->right = &x;
        }
        x.right = &y;
        y.parent = &x;
    }

    constexpr Iterator begin_impl() const { return Iterator(m_minimum, !m_root); }
    constexpr Iterator end_impl() const { return Iterator(m_maximum, true); }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr View<Iterator> equal_range_impl(U const& needle) const {
        return { lower_bound_impl(needle), upper_bound_impl(needle) };
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr Iterator lower_bound_impl(U const& needle) const {
        Node* result = nullptr;
        for (auto* node = m_root; node;) {
            if (compare(node->value, needle) < 0) {
                node = node->right;
            } else {
                result = node;
                node = node->left;
            }
        }
        return result ? Iterator(result, false) : end_impl();
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr Iterator upper_bound_impl(U const& needle) const {
        Node* result = nullptr;
        for (auto* node = m_root; node;) {
            if (compare(node->value, needle) <= 0) {
                node = node->right;
            } else {
                result = node;
                node = node->left;
            }
        }
        return result ? Iterator(result, false) : end_impl();
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr Iterator find_impl(U const& needle) const {
        for (auto* node = m_root; node;) {
            auto result = compare(needle, node->value);
            if (result == 0) {
                return Iterator(node, false);
            } else if (result < 0) {
                node = node->left;
            } else {
                node = node->right;
            }
        }
        return end_impl();
    }

    constexpr void insert_node(Node& to_insert) {
        // Step 1: find the parent node to insert under.
        Node* y = nullptr;
        auto* x = m_root;
        while (x != nullptr) {
            y = x;
            if (compare(to_insert, *x) < 0) {
                x = x->left;
            } else {
                x = x->right;
            }
        }

        // Step 2: actually insert the node.
        to_insert.parent = y;
        if (y == nullptr) {
            m_root = m_minimum = m_maximum = &to_insert;
        } else if (compare(to_insert, *y) < 0) {
            y->left = &to_insert;
        } else {
            y->right = &to_insert;
        }

        // Step 3: maintain the Red-Black properties.
        do_insert_rebalancing(&to_insert);

        // Step 4: update cached values.
        m_size++;
        if (m_minimum->left) {
            m_minimum = &to_insert;
        }
        if (m_maximum->right) {
            m_maximum = &to_insert;
        }
    }

    constexpr void do_insert_rebalancing(Node* node) {
        DI_ASSERT(node);
        while (node->parent && node->parent->parent && node->parent->color == Node::Color::Red) {
            auto* grand_parent = node->parent->parent;
            if (node->parent->is_right_child()) {
                auto* uncle = grand_parent->left;
                if (node_color(uncle) == Node::Color::Red) {
                    node->parent->color = Node::Color::Black;
                    uncle->color = Node::Color::Black;
                    grand_parent->color = Node::Color::Red;
                    node = grand_parent;
                } else {
                    if (node->is_left_child()) {
                        node = node->parent;
                        rotate_right(*node);
                    }
                    node->parent->color = Node::Color::Black;
                    grand_parent->color = Node::Color::Red;
                    rotate_left(*grand_parent);
                }
            } else {
                auto* uncle = grand_parent->right;
                if (node_color(uncle) == Node::Color::Red) {
                    node->parent->color = Node::Color::Black;
                    uncle->color = Node::Color::Black;
                    grand_parent->color = Node::Color::Red;
                    node = grand_parent;
                } else {
                    if (node->is_right_child()) {
                        node = node->parent;
                        rotate_left(*node);
                    }
                    node->parent->color = Node::Color::Black;
                    grand_parent->color = Node::Color::Red;
                    rotate_right(*grand_parent);
                }
            }
        }
        m_root->color = Node::Color::Black;
    }

    constexpr void transplant(Node& u, Node* v) {
        if (u.parent == nullptr) {
            m_root = v;
        } else if (u.is_left_child()) {
            u.parent->left = v;
        } else {
            u.parent->right = v;
        }
        if (v) {
            v->parent = u.parent;
        }
    }

    constexpr void erase_node(Node& to_delete) {
        Node* x = nullptr;
        auto* y = &to_delete;
        auto y_color = y->color;

        // Step 1: Update cached values.
        m_size--;
        if (m_minimum == &to_delete) {
            m_minimum = to_delete.successor();
        }
        if (m_maximum == &to_delete) {
            m_maximum = to_delete.predecessor();
        }

        // Step 2: actually remove the node from the tree.
        if (to_delete.left == nullptr) {
            // Case 1: there is no left child, so promote the right child.
            x = to_delete.right;
            transplant(to_delete, to_delete.right);
        } else if (to_delete.right == nullptr) {
            // Case 2: there is no right child, so promote the left child.
            x = to_delete.left;
            transplant(to_delete, to_delete.left);
        } else {
            // Case 3: promote this node's successor
            y = to_delete.successor();
            y_color = y->color;
            x = y->right;

            if (y->parent == &to_delete) {
                if (x) {
                    x->parent = y;
                }
            } else {
                transplant(*y, y->right);
                y->right = to_delete.right;
                y->right->parent = y;
            }
            transplant(to_delete, y);
            y->left = to_delete.left;
            y->left->parent = y;
            y->color = to_delete.color;
        }

        // Step 3: maintain the Red-Black properties.
        if (y_color == Node::Color::Black && x) {
            do_erase_rebalancing(x);
        }
    }

    constexpr void do_erase_rebalancing(Node* x) {
        DI_ASSERT(x);
        while (x != m_root && x->color == Node::Color::Black) {
            if (x->is_left_child()) {
                auto* w = x->parent->right;
                DI_ASSERT(w);
                if (w->color == Node::Color::Red) {
                    x->color = Node::Color::Black;
                    x->parent->color = Node::Color::Red;
                    rotate_left(*x->parent);
                }
                if (node_color(w->left) == Node::Color::Black && node_color(w->right) == Node::Color::Black) {
                    w->color = Node::Color::Red;
                    x = x->parent;
                } else {
                    if (node_color(w->right) == Node::Color::Black) {
                        w->left->color = Node::Color::Black;
                        w->color = Node::Color::Red;
                        rotate_right(*w);
                        w = x->parent->right;
                    }
                    w->color = x->parent->color;
                    x->parent->color = Node::Color::Black;
                    w->right->color = Node::Color::Black;
                    rotate_left(*x->parent);
                    break;
                }
            } else {
                auto* w = x->parent->left;
                if (!w) {
                    x = x->parent;
                    continue;
                }
                if (w->color == Node::Color::Red) {
                    x->color = Node::Color::Black;
                    x->parent->color = Node::Color::Red;
                    rotate_right(*x->parent);
                }
                if (node_color(w->right) == Node::Color::Black && node_color(w->left) == Node::Color::Black) {
                    w->color = Node::Color::Red;
                    x = x->parent;
                } else {
                    if (node_color(w->left) == Node::Color::Black) {
                        w->right->color = Node::Color::Black;
                        w->color = Node::Color::Red;
                        rotate_left(*w);
                        w = x->parent->left;
                    }
                    w->color = x->parent->color;
                    x->parent->color = Node::Color::Black;
                    w->left->color = Node::Color::Black;
                    rotate_right(*x->parent);
                    break;
                }
            }
        }
        x->color = Node::Color::Black;
    }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr Node* create_node(Args&&... args) {
        auto [pointer, allocated_nodes] = Alloc().allocate(1);
        (void) allocated_nodes;

        util::construct_at(pointer, in_place, util::forward<Args>(args)...);
        return pointer;
    }

    constexpr void destroy_node(Node& node) {
        util::destroy_at(&node);
        Alloc().deallocate(&node, 1);
    }

    constexpr auto compare(Node const& a, Node const& b) const { return compare(a.value, b.value); }
    constexpr auto compare(Value const& a, Value const& b) const { return function::invoke(m_comparator, a, b); }

    Node* m_root { nullptr };
    Node* m_minimum { nullptr };
    Node* m_maximum { nullptr };
    size_t m_size { 0 };
    [[no_unique_address]] Comp m_comparator;
};
}