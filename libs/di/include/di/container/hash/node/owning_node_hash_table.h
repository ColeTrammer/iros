#pragma once

#include <di/concepts/predicate.h>
#include <di/container/allocator/allocator_of.h>
#include <di/container/hash/hash_same.h>
#include <di/container/hash/hasher.h>
#include <di/container/hash/node/hash_node.h>
#include <di/container/hash/node/node_hash_table.h>
#include <di/container/intrusive/intrusive_tag_base.h>
#include <di/container/vector/mutable_vector.h>
#include <di/meta/like_expected.h>

namespace di::container {
template<typename Self, typename T>
struct OwningHashNodeTag;

template<typename T, typename Tag>
struct OwningHashNode : HashNode<Tag> {
public:
    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr OwningHashNode(InPlace, Args&&... args) : m_value(util::forward<Args>(args)...) {}

    constexpr T& value() { return m_value; }

private:
    T m_value;
};

template<typename Self, typename T>
struct OwningHashNodeTag : IntrusiveTagBase<OwningHashNode<T, Self>> {
    using Node = OwningHashNode<T, Self>;

    constexpr static bool is_sized(auto) { return false; }
    constexpr static OwningHashNode<T, Self>& down_cast(auto, auto& node) {
        return static_cast<OwningHashNode<T, Self>&>(node);
    }
    constexpr static T& down_cast(InPlaceType<T>, Node& node) { return node.value(); }

    constexpr static void did_remove(auto& self, auto& node) {
        if constexpr (requires { self.allocator().deallocate(util::addressof(node), 1); }) {
            util::destroy_at(util::addressof(node));
            self.allocator().deallocate(util::addressof(node), 1);
        }
    }
};

template<typename Value, typename Eq, concepts::Hasher Hasher, typename Buckets, typename Tag,
         concepts::AllocatorOf<OwningHashNode<Value, Tag>> Alloc, typename Interface, bool is_multi>
class OwningNodeHashTable
    : public NodeHashTable<Value, Eq, Hasher, Buckets, Tag, Interface, is_multi,
                           OwningNodeHashTable<Value, Eq, Hasher, Buckets, Tag, Alloc, Interface, is_multi>> {
private:
    using Base = NodeHashTable<Value, Eq, Hasher, Buckets, Tag, Interface, is_multi,
                               OwningNodeHashTable<Value, Eq, Hasher, Buckets, Tag, Alloc, Interface, is_multi>>;

    using Node = HashNode<Tag>;
    using Iterator = HashNodeIterator<Value, Tag>;
    using ConstIterator = meta::ConstIterator<Iterator>;

    using AllocResult = decltype(Alloc().allocate(0));

    template<typename T>
    using Result = meta::LikeExpected<AllocResult, T>;

public:
    using Base::Base;

    constexpr Alloc allocator() const { return Alloc(); }

    template<typename U, concepts::Invocable F>
    requires(concepts::Predicate<Eq&, Value const&, U const&> && concepts::HashSame<Value, U> &&
             concepts::MaybeFallible<meta::InvokeResult<F>, Value>)
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
    requires(concepts::Predicate<Eq&, Value const&, U const&> && concepts::HashSame<Value, U> &&
             concepts::MaybeFallible<meta::InvokeResult<F>, Value>)
    constexpr auto insert_with_factory(ConstIterator, U&& needle, F&& factory) {
        return this->insert_with_factory(util::forward<U>(needle), util::forward<F>(factory));
    }

private:
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto create_node(Args&&... args) {
        return as_fallible(Alloc().allocate(1)) % [&](Allocation<OwningHashNode<Value, Tag>> allocation) {
            auto [pointer, allocated_nodes] = allocation;
            (void) allocated_nodes;

            util::construct_at(pointer, in_place, util::forward<Args>(args)...);
            return static_cast<Node*>(pointer);
        } | try_infallible;
    }
};
}
