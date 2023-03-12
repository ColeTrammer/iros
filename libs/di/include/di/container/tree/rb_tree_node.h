#pragma once

#include <di/concepts/constructible_from.h>
#include <di/types/prelude.h>
#include <di/util/forward.h>

namespace di::container {
template<typename Tag>
struct RBTreeNode {
public:
    enum class Color { Red = 0, Black = 1 };

    RBTreeNode() = default;

    constexpr bool is_left_child() const { return parent && parent->left == this; }
    constexpr bool is_right_child() const { return parent && parent->right == this; }

    constexpr RBTreeNode& find_min() {
        auto* node = this;
        while (node->left) {
            node = node->left;
        }
        return *node;
    }

    constexpr RBTreeNode& find_max() {
        auto* node = this;
        while (node->right) {
            node = node->right;
        }
        return *node;
    }

    constexpr RBTreeNode* predecessor() const {
        if (left) {
            return &left->find_max();
        }

        auto* child = this;
        auto* parent = this->parent;
        while (parent && child->is_left_child()) {
            child = parent;
            parent = parent->parent;
        }
        return parent;
    }

    constexpr RBTreeNode* successor() const {
        if (right) {
            return &right->find_min();
        }

        auto* child = this;
        auto* parent = this->parent;
        while (parent && child->is_right_child()) {
            child = parent;
            parent = parent->parent;
        }
        return parent;
    }

    Color color { Color::Red };
    RBTreeNode* parent { nullptr };
    RBTreeNode* left { nullptr };
    RBTreeNode* right { nullptr };
};
}
