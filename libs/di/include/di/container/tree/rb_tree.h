#pragma once

#include <di/assert/assert_bool.h>
#include <di/concepts/equality_comparable.h>
#include <di/concepts/three_way_comparable.h>
#include <di/container/allocator/prelude.h>
#include <di/container/concepts/prelude.h>
#include <di/container/iterator/distance.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/const_iterator.h>
#include <di/container/tree/rb_tree_iterator.h>
#include <di/container/tree/rb_tree_node.h>
#include <di/function/compare.h>
#include <di/meta/conditional.h>
#include <di/util/create.h>
#include <di/util/exchange.h>
#include <di/vocab/optional/prelude.h>

namespace di::container {
namespace detail {
    template<typename Value, typename Comp>
    struct RBTreeValidForLookup {
        template<typename U>
        struct Type {
            constexpr static inline bool value = concepts::StrictWeakOrder<Comp&, Value, U>;
        };
    };
}

/// @brief General implementation of the Red-Black self-balancing binary tree.
///
/// The book Introduction to Algorithms, Third Edition (by Thomas H. Cormen, et al.)
/// was heavily referenced in this class's implementation of a Red-Black tree.
/// See [here](https://mitpress.mit.edu/9780262046305/introduction-to-algorithms/).
template<typename Value, typename Comp, typename Tag, typename Interface, bool is_multi, typename Self = Void>
class RBTree : public Interface {
private:
    using Node = RBTreeNode<Tag>;
    using Iterator = RBTreeIterator<Value, Tag>;
    using ConstIterator = container::ConstIteratorImpl<Iterator>;

    using ConcreteNode = decltype(Tag::node_type(in_place_type<Value>));
    using ConcreteSelf = meta::Conditional<concepts::SameAs<Void, Self>, RBTree, Self>;

    constexpr decltype(auto) down_cast_self() {
        if constexpr (concepts::SameAs<Void, Self>) {
            return *this;
        } else {
            return static_cast<Self&>(*this);
        }
    }

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

    constexpr ~RBTree() { this->clear(); }

    constexpr usize size() const { return m_size; }
    constexpr bool empty() const { return !m_root; }

    constexpr Iterator begin() { return unconst_iterator(util::as_const(*this).begin()); }
    constexpr ConstIterator begin() const { return Iterator(m_minimum, !m_root); }
    constexpr Iterator end() { return unconst_iterator(util::as_const(*this).end()); }
    constexpr ConstIterator end() const { return Iterator(m_maximum, true); }

    constexpr Iterator unconst_iterator(ConstIterator it) { return it.base(); }

    constexpr auto insert_node(Node& node) {
        auto position = this->insert_position(node_value(node));
        if constexpr (!is_multi) {
            if (position.parent && this->compare(node_value(*position.parent), node_value(node)) == 0) {
                return Tuple(Iterator(position.parent, false), false);
            }
        }

        this->insert_node(position, node);
        if constexpr (!is_multi) {
            return Tuple(Iterator(util::addressof(node), false), true);
        } else {
            return Iterator(util::addressof(node), false);
        }
    }

    constexpr auto insert_node(ConstIterator, Node& node) {
        auto position = this->insert_position(node_value(node));
        if constexpr (!is_multi) {
            if (position.parent && this->compare(node_value(*position.parent), node_value(node)) == 0) {
                return Iterator(position.parent, false);
            }
        }

        this->insert_node(position, node);
        return Iterator(util::addressof(node), false);
    }

    constexpr Iterator erase_impl(ConstIterator position) {
        DI_ASSERT(position != end());

        auto result = container::next(position).base();
        auto& node = position.base().node();
        erase_node(node);

        Tag::did_remove(down_cast_self(), static_cast<ConcreteNode&>(node));

        return result;
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr auto equal_range_impl(U&& needle) const {
        return View<ConstIterator> { lower_bound_impl(needle), upper_bound_impl(needle) };
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr ConstIterator lower_bound_impl(U&& needle) const {
        Node* result = nullptr;
        for (auto* node = m_root; node;) {
            if (compare(node_value(*node), needle) < 0) {
                node = node->right;
            } else {
                result = node;
                node = node->left;
            }
        }
        return result ? Iterator(result, false) : end();
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr ConstIterator upper_bound_impl(U&& needle) const {
        Node* result = nullptr;
        for (auto* node = m_root; node;) {
            if (compare(node_value(*node), needle) <= 0) {
                node = node->right;
            } else {
                result = node;
                node = node->left;
            }
        }
        return result ? Iterator(result, false) : end();
    }

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr ConstIterator find_impl(U&& needle) const {
        for (auto* node = m_root; node;) {
            auto result = compare(needle, node_value(*node));
            if (result == 0) {
                return Iterator(node, false);
            } else if (result < 0) {
                node = node->left;
            } else {
                node = node->right;
            }
        }
        return end();
    }

    constexpr void merge_impl(RBTree&& other) {
        if (!m_root) {
            *this = util::move(other);
            return;
        }

        auto it = other.begin();
        auto last = other.end();
        while (it != last) {
            auto save = it++;

            auto position = insert_position(*save);
            if constexpr (is_multi) {
                do_insert_node(position, save.node());
            } else if (position.parent && compare(position.parent->value, *save) == 0) {
                erase_impl(save);
            } else {
                do_insert_node(position, save.node());
            }
        }
    }

protected:
    constexpr Value& node_value(Node& node) const {
        return Tag::down_cast(in_place_type<Value>, static_cast<ConcreteNode&>(node));
    }
    constexpr Value const& node_value(Node const& node) const {
        return const_cast<RBTree&>(*this).node_value(const_cast<Node&>(node));
    }

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

    struct InsertPosition {
        Node* parent { nullptr };
        bool left { true };
    };

    template<typename U>
    requires(concepts::StrictWeakOrder<Comp&, Value, U>)
    constexpr InsertPosition insert_position(U&& needle) const {
        // Find the parent node to insert under.
        Node* y = nullptr;
        auto* x = m_root;
        while (x != nullptr) {
            y = x;

            // Early return to the caller if we're inserting a duplicate, making
            // sure to provide the caller a node that is equal to needle.
            if constexpr (!is_multi) {
                if (compare(needle, node_value(*x)) == 0) {
                    return InsertPosition { x, false };
                }
            }

            if (compare(needle, node_value(*x)) < 0) {
                x = x->left;
            } else {
                x = x->right;
            }
        }

        if (!y) {
            return InsertPosition {};
        }
        if (compare(needle, node_value(*y)) < 0) {
            return InsertPosition { y, true };
        }
        return InsertPosition { y, false };
    }

    constexpr void insert_node(InsertPosition position, Node& to_insert) {
        do_insert_node(position, to_insert);

        Tag::did_insert(down_cast_self(), static_cast<ConcreteNode&>(to_insert));
    }

    constexpr void do_insert_node(InsertPosition position, Node& to_insert) {
        // Step 1: check if inserting the first node.
        if (position.parent == nullptr) {
            m_root = m_minimum = m_maximum = &to_insert;
            m_size = 1;
            return;
        }

        // Step 2: actually insert the node.
        auto& parent = *position.parent;
        to_insert.parent = &parent;
        if (position.left) {
            parent.left = &to_insert;
        } else {
            parent.right = &to_insert;
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
                // FIXME: this NULL check appears to be necessary, but is not part of the reference implementation.
                if (!w) {
                    break;
                }

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
                // FIXME: this NULL check appears to be necessary, but is not part of the reference implementation.
                if (!w) {
                    break;
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

    constexpr auto compare(Node const& a, Node const& b) const { return compare(node_value(a), node_value(b)); }

    template<typename T, typename U>
    requires(concepts::StrictWeakOrder<Comp&, T, U>)
    constexpr auto compare(T const& a, U&& b) const {
        return function::invoke(m_comparator, a, b);
    }

    constexpr friend bool operator==(ConcreteSelf const& a, ConcreteSelf const& b)
    requires(concepts::EqualityComparable<Value>)
    {
        return container::equal(a, b);
    }
    constexpr friend auto operator<=>(ConcreteSelf const& a, ConcreteSelf const& b)
    requires(concepts::ThreeWayComparable<Value>)
    {
        return container::compare(a, b);
    }

    Node* m_root { nullptr };
    Node* m_minimum { nullptr };
    Node* m_maximum { nullptr };
    usize m_size { 0 };
    [[no_unique_address]] Comp m_comparator;
};
}
