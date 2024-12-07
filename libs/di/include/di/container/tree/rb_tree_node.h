#pragma once

#include "di/meta/operations.h"
#include "di/types/prelude.h"
#include "di/util/forward.h"

namespace di::container {
template<typename Tag>
struct RBTreeNode {
public:
    enum class Color { Red = 0, Black = 1 };

    RBTreeNode() = default;

    constexpr auto is_left_child() const -> bool { return parent && parent->left == this; }
    constexpr auto is_right_child() const -> bool { return parent && parent->right == this; }

    constexpr auto find_min() -> RBTreeNode& {
        auto* node = this;
        while (node->left) {
            node = node->left;
        }
        return *node;
    }

    constexpr auto find_max() -> RBTreeNode& {
        auto* node = this;
        while (node->right) {
            node = node->right;
        }
        return *node;
    }

    constexpr auto predecessor() const -> RBTreeNode* {
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

    constexpr auto successor() const -> RBTreeNode* {
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
