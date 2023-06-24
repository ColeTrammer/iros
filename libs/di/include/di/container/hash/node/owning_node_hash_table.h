#pragma once

#include <di/container/allocator/allocate_one.h>
#include <di/container/allocator/allocation_result.h>
#include <di/container/allocator/allocator.h>
#include <di/container/allocator/deallocate_one.h>
#include <di/container/hash/hash_same.h>
#include <di/container/hash/hasher.h>
#include <di/container/hash/node/hash_node.h>
#include <di/container/hash/node/node_hash_table.h>
#include <di/container/intrusive/intrusive_tag_base.h>
#include <di/container/vector/mutable_vector.h>
#include <di/meta/core.h>
#include <di/meta/relation.h>
#include <di/meta/vocab.h>
#include <di/vocab/expected/prelude.h>

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
        if constexpr (requires { di::deallocate_one<Node>(self.allocator(), util::addressof(node)); }) {
            util::destroy_at(util::addressof(node));
            di::deallocate_one<Node>(self.allocator(), util::addressof(node));
        }
    }

    constexpr static bool allow_rehashing_in_insert(auto) { return true; }
};

template<typename Value, typename Eq, concepts::Hasher Hasher, typename Buckets, typename Tag,
         concepts::Allocator Alloc, typename Interface, bool is_multi, bool is_map>
class OwningNodeHashTable
    : public NodeHashTable<Value, Eq, Hasher, Buckets, Tag, Interface, is_multi, is_map,
                           OwningNodeHashTable<Value, Eq, Hasher, Buckets, Tag, Alloc, Interface, is_multi, is_map>> {
private:
    using Base =
        NodeHashTable<Value, Eq, Hasher, Buckets, Tag, Interface, is_multi, is_map,
                      OwningNodeHashTable<Value, Eq, Hasher, Buckets, Tag, Alloc, Interface, is_multi, is_map>>;

    using Node = HashNode<Tag>;
    using Iterator = HashNodeIterator<Value, Tag>;
    using ConstIterator = container::ConstIteratorImpl<Iterator>;

    using AllocResult = meta::AllocatorResult<Alloc>;

    template<typename T>
    using Result = meta::LikeExpected<AllocResult, T>;

    using InsertResult =
        meta::LikeExpected<AllocResult, meta::Conditional<is_multi, Iterator, vocab::Tuple<Iterator, bool>>>;

public:
    using Base::Base;

    constexpr Alloc& allocator() { return m_allocator; }

    template<typename U, concepts::Invocable F>
    constexpr InsertResult insert_with_factory(U&& needle, F&& factory) {
        if (this->bucket_count() == 0) {
            if constexpr (concepts::Expected<AllocResult>) {
                DI_TRY(this->reserve(20));
            } else {
                this->reserve(20);
            }
        }

        auto const hash = this->hash(needle);
        auto const bucket_index = hash % this->bucket_count();
        auto* bucket = &vector::lookup(this->m_buckets, bucket_index);

        auto before_it = bucket->before_begin();
        while (container::next(before_it) != bucket->end()) {
            auto&& current = *container::next(before_it);
            if (this->equal(this->node_value(current), needle)) {
                break;
            }
            ++before_it;
        }
        auto it = container::next(before_it);

        auto do_insert = [&] {
            if constexpr (is_multi) {
                return true;
            } else {
                return it == bucket->end();
            }
        }();

        if (do_insert) {
            if (this->size() + 1 >= this->bucket_count()) {
                auto new_capacity = 2 * this->bucket_count();
                if constexpr (concepts::Expected<AllocResult>) {
                    DI_TRY(this->reserve(new_capacity));
                } else {
                    this->reserve(new_capacity);
                }
            }
            bucket = &vector::lookup(this->m_buckets, bucket_index);
            before_it = bucket->before_begin();
            it = container::next(before_it);
            ++this->m_size;
        } else if constexpr (!is_multi) {
            return vocab::Tuple(Iterator { this->m_buckets.span(), bucket_index, before_it }, false);
        }

        return as_fallible(this->create_node(function::invoke(util::forward<F>(factory)))) % [&](auto* node) {
            if constexpr (is_multi) {
                if (it != bucket->end()) {
                    bucket->insert_after(it, *node);
                    Tag::did_insert(this->down_cast_self(), static_cast<Base::ConcreteNode&>(*node));
                    return Iterator { this->m_buckets.span(), bucket_index, it };
                }
                bucket->push_front(*node);
                Tag::did_insert(this->down_cast_self(), static_cast<Base::ConcreteNode&>(*node));
                return Iterator { this->m_buckets.span(), bucket_index, bucket->before_begin() };
            } else {
                if (it == bucket->end()) {
                    bucket->push_front(*node);
                    Tag::did_insert(this->down_cast_self(), static_cast<Base::ConcreteNode&>(*node));
                    return vocab::Tuple(Iterator { this->m_buckets.span(), bucket_index, bucket->before_begin() },
                                        true);
                }
                return vocab::Tuple(Iterator { this->m_buckets.span(), bucket_index, before_it }, false);
            }
        } | try_infallible;
    }

    template<typename U, concepts::Invocable F>
    constexpr auto insert_with_factory(ConstIterator, U&& needle, F&& factory) {
        if constexpr (!is_multi) {
            return as_fallible(this->insert_with_factory(util::forward<U>(needle), util::forward<F>(factory))) %
                       [](auto&& result) {
                           return util::get<0>(result);
                       } |
                   try_infallible;
        } else {
            return this->insert_with_factory(util::forward<U>(needle), util::forward<F>(factory));
        }
    }

private:
    template<typename... Args>
    requires(concepts::ConstructibleFrom<Value, Args...>)
    constexpr auto create_node(Args&&... args) {
        return as_fallible(di::allocate_one<OwningHashNode<Value, Tag>>(m_allocator)) %
                   [&](OwningHashNode<Value, Tag>* pointer) {
                       util::construct_at(pointer, in_place, util::forward<Args>(args)...);
                       return static_cast<Node*>(pointer);
                   } |
               try_infallible;
    }

    [[no_unique_address]] Alloc m_allocator {};
};
}
