#pragma once

namespace di::container {
template<typename Value>
struct RBTreeNode {
public:
    enum class Color { Red = 0, Black = 1 };

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

    Color color { Color::Red };
    RBTreeNode* parent { nullptr };
    RBTreeNode* left { nullptr };
    RBTreeNode* right { nullptr };
    [[no_unique_address]] Value value;
};
}