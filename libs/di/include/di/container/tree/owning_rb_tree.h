#pragma once

#include <di/container/allocator/allocate_one.h>
#include <di/container/allocator/allocation_result.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/deallocate_one.h>
#include <di/container/intrusive/intrusive_tag_base.h>
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
        di::deallocate_one<Node>(self.allocator(), util::addressof(node));
    }
};

template<typename Value, typename Comp, typename Tag, concepts::Allocator Alloc, typename Interface, bool is_multi>
class OwningRBTree
    : public RBTree<Value, Comp, Tag, Interface, is_multi, OwningRBTree<Value, Comp, Tag, Alloc, Interface, is_multi>> {
private:
    using Base =
        RBTree<Value, Comp, Tag, Interface, is_multi, OwningRBTree<Value, Comp, Tag, Alloc, Interface, is_multi>>;

    using Node = RBTreeNode<Tag>;
    using Iterator = RBTreeIterator<Value, Tag>;
    using ConstIterator = container::ConstIteratorImpl<Iterator>;

    using AllocResult = meta::AllocatorResult<Alloc>;

    template<typename T>
    using Result = meta::LikeExpected<AllocResult, T>;

public:
    using Base::Base;

    constexpr Alloc& allocator() { return m_allocator; }

    template<typename U, concepts::Invocable F>
    requires(concepts::StrictWeakOrder<Comp&, Value, U> && concepts::MaybeFallible<meta::InvokeResult<F>, Value>)
    constexpr auto insert_with_factory(U&& needle, F&& factory) {
        auto position = this->insert_position(needle);
        if constexpr (!is_multi) {
            if (position.parent && this->compare(this->node_value(*position.parent), needle) == 0) {
                return Result<Tuple<Iterator, bool>>(Tuple(Iterator(position.parent, false), false));
            }
        }

        return as_fallible(this->create_node(function::invoke(util::forward<F>(factory)))) % [&](auto* node) {
            this->insert_node(position, *node);
            if constexpr (!is_multi) {
                return Tuple(Iterator(node, false), true);
            } else {
                return Iterator(node, false);
            }
        } | try_infallible;
    }

    template<typename U, concepts::Invocable F>
    requires(concepts::StrictWeakOrder<Comp&, Value, U> && concepts::MaybeFallible<meta::InvokeResult<F>, Value>)
    constexpr auto insert_with_factory(ConstIterator, U&& needle, F&& factory) {
        auto position = this->insert_position(needle);
        if constexpr (!is_multi) {
            if (position.parent && this->compare(this->node_value(*position.parent), needle) == 0) {
                return Result<Tuple<Iterator, bool>>(Tuple(Iterator(position.parent, false), false));
            }
        }

        return as_fallible(this->create_node(function::invoke(util::forward<F>(factory)))) % [&](auto* node) {
            this->insert_node(position, *node);
            return Iterator(node, false);
        } | try_infallible;
    }

private:
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto create_node(Args&&... args) {
        return as_fallible(di::allocate_one<OwningRBTreeNode<Value, Tag>>(m_allocator)) %
                   [&](OwningRBTreeNode<Value, Tag>* pointer) {
                       util::construct_at(pointer, in_place, util::forward<Args>(args)...);
                       return static_cast<Node*>(pointer);
                   } |
               try_infallible;
    }

    [[no_unique_address]] Alloc m_allocator {};
};
}
