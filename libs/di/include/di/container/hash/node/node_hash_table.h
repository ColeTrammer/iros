#pragma once

#include <di/vocab/expected/prelude.h>

#include <di/assert/assert_bool.h>
#include <di/concepts/predicate.h>
#include <di/container/algorithm/find_if.h>
#include <di/container/hash/hash.h>
#include <di/container/hash/hash_same.h>
#include <di/container/hash/hasher.h>
#include <di/container/hash/node/hash_node.h>
#include <di/container/hash/node/hash_node_iterator.h>
#include <di/container/vector/mutable_vector.h>
#include <di/function/monad/monad_try.h>
#include <di/function/not_fn.h>
#include <di/util/declval.h>
#include <di/util/get.h>
#include <di/vocab/expected/prelude.h>

namespace di::container {
namespace detail {
    template<typename Value, typename Eq>
    struct NodeHashTableValidForLookup {
        template<typename U>
        struct Type {
            constexpr static inline bool value =
                concepts::Predicate<Eq&, Value const&, U const&> && concepts::HashSame<Value, U>;
        };
    };
}

/// @brief Node based (closed addressing) hash table.
///
/// This is fairly straightforward implementation of a hash table. It uses a vector of buckets, where each bucket is a
/// singlely-linked list of nodes.
template<typename Value, typename Eq, concepts::Hasher Hasher, typename Buckets, typename Tag, typename Interface,
         bool is_multi, typename Self = Void>
class NodeHashTable : public Interface {
private:
    using Node = HashNode<Tag>;
    using Iterator = HashNodeIterator<Value, Tag>;
    using ConstIterator = meta::ConstIterator<Iterator>;

    using ConcreteNode = decltype(Tag::node_type(in_place_type<Value>));

    constexpr decltype(auto) down_cast_self() {
        if constexpr (concepts::SameAs<Void, Self>) {
            return *this;
        } else {
            return static_cast<Self&>(*this);
        }
    }

    constexpr static bool allow_rehashing_in_insert = false;

public:
    NodeHashTable() = default;
    NodeHashTable(NodeHashTable const&) = delete;
    NodeHashTable& operator=(NodeHashTable const&) = delete;

    constexpr explicit NodeHashTable(Eq eq, Hasher hasher = {}) : m_eq(util::move(eq)), m_hasher(util::move(hasher)) {}

    constexpr NodeHashTable(NodeHashTable&& other)
        : m_buckets(util::move(other.m_buckets))
        , m_size(util::exchange(other.m_size, 0))
        , m_eq(util::move(other.m_eq))
        , m_hasher(util::move(other.m_hasher)) {}

    constexpr NodeHashTable& operator=(NodeHashTable&& other) {
        this->clear();
        m_buckets = util::move(other.m_buckets);
        m_size = util::exchange(other.m_size, 0);
        m_eq = util::move(other.m_eq);
        m_hasher = util::move(other.m_hasher);
        return *this;
    }

    constexpr ~NodeHashTable() { this->clear(); }

    constexpr usize size() const { return m_size; }
    constexpr bool empty() const { return m_size == 0; }

    constexpr Iterator begin() { return unconst_iterator(util::as_const(*this).begin()); }
    constexpr ConstIterator begin() const {
        auto const buckets = m_buckets.span();
        auto it = container::find_if(m_buckets, function::not_fn(container::empty));
        if (it == m_buckets.end()) {
            return {};
        }
        auto const bucket_index = it - buckets.begin();
        return Iterator(buckets, bucket_index);
    }
    constexpr Iterator end() { return unconst_iterator(util::as_const(*this).end()); }
    constexpr ConstIterator end() const { return Iterator(m_buckets.span(), m_buckets.size()); }

    constexpr Iterator unconst_iterator(ConstIterator it) { return it.base(); }

    constexpr auto insert_node(Node& node)
    requires(!allow_rehashing_in_insert)
    {
        return insert_node_without_rehashing(node);
    }

    constexpr auto insert_node(ConstIterator, Node& node) {
        if constexpr (!is_multi) {
            return as_fallible(this->insert_node(node)) % [](auto&& tuple) {
                return util::get<0>(tuple);
            } | try_infallible;
        } else {
            return this->insert_node(node);
        }
    }

    constexpr Iterator erase_impl(ConstIterator it) {
        auto const bucket_index = it.base().bucket_index();
        auto& bucket = m_buckets[bucket_index];
        auto const next = bucket.erase_after(it.base().before_current());
        --m_size;

        Tag::did_remove(down_cast_self(), static_cast<ConcreteNode&>(it.base().node()));
        return Iterator { m_buckets.span(), bucket_index, next };
    }

    template<typename U>
    requires(concepts::Predicate<Eq&, Value const&, U const&> && concepts::HashSame<Value, U>)
    constexpr View<ConstIterator> equal_range_impl(U&& needle) const {
        if (empty()) {
            return { end(), end() };
        }

        auto const hash = this->hash(needle);
        auto const bucket_index = hash % m_buckets.size();
        auto const& bucket = m_buckets[bucket_index];
        auto before_it = bucket.before_begin();
        while (container::next(before_it) != bucket.end()) {
            auto&& current = *container::next(before_it);
            if (this->equal(current, needle)) {
                break;
            }
            ++before_it;
        }

        auto it = container::next(before_it);
        if (it == bucket.end()) {
            return { end(), end() };
        }

        auto const first = ConstIterator { m_buckets.span(), bucket_index, before_it };
        auto last = ConstIterator { m_buckets.span(), bucket_index, it };
        if constexpr (is_multi) {
            while (container::next(last.before_current()) != bucket.end()) {
                auto&& current = *container::next(last.before_current());
                if (!this->equal(node_value(current), needle)) {
                    break;
                }
                ++last;
            }
        }
        return { first, last };
    }

    constexpr auto reserve(usize new_capacity)
        -> decltype(util::declval<Buckets&>().reserve_from_nothing(new_capacity)) {
        if (vector::empty(m_buckets)) {
            return m_buckets.reserve_from_nothing(new_capacity);
        }

        if (new_capacity <= m_buckets.size()) {
            return;
        }

        auto new_map = NodeHashTable {};
        if constexpr (concepts::LanguageVoid<decltype(util::declval<Buckets&>().reserve_from_nothing(new_capacity))>) {
            new_map.m_buckets.reserve_from_nothing(new_capacity);
        } else {
            DI_TRY(new_map.m_buckets.reserve_from_nothing(new_capacity));
        }

        auto it = this->begin();
        auto const end = this->end();
        for (; it != end;) {
            auto& node = it.node();
            ++it;
            new_map.insert_node_without_rehashing(node);
        }
        if constexpr (concepts::LanguageVoid<decltype(util::declval<Buckets&>().reserve_from_nothing(new_capacity))>) {
            return;
        } else {
            return {};
        }
    }

    constexpr void merge_impl(NodeHashTable&& other)
    requires(!allow_rehashing_in_insert)
    {
        merge_impl_without_rehashing(util::move(other));
    }

protected:
    constexpr Value& node_value(Node& node) const {
        return Tag::down_cast(in_place_type<Value>, static_cast<ConcreteNode&>(node));
    }
    constexpr Value const& node_value(Node const& node) const {
        return const_cast<NodeHashTable&>(*this).node_value(const_cast<Node&>(node));
    }

private:
    constexpr void merge_impl_without_rehashing(NodeHashTable&& other) {
        auto it = other.begin();
        auto const end = other.end();
        for (; it != end;) {
            auto& node = it.node();
            ++it;
            this->insert_node_without_rehashing(node);
        }
    }

    constexpr auto insert_node_without_rehashing(Node& node) {
        DI_ASSERT(!vector::empty(m_buckets));

        auto const hash = this->hash(node_value(node));
        auto const bucket_index = hash % m_buckets.size();
        auto& bucket = m_buckets[bucket_index];

        auto before_it = bucket.before_begin();
        while (container::next(before_it) != bucket.end()) {
            auto&& current = *container::next(before_it);
            if (this->equal(node_value(current), node_value(node))) {
                break;
            }
            ++before_it;
        }
        auto it = container::next(before_it);
        if constexpr (is_multi) {
            ++m_size;
            if (it != bucket.end()) {
                bucket.insert_after(it, node);
                Tag::did_insert(down_cast_self(), static_cast<ConcreteNode&>(node));
                return Iterator { m_buckets.span(), bucket_index, it };
            } else {
                bucket.push_front(node);
                Tag::did_insert(down_cast_self(), static_cast<ConcreteNode&>(node));
                return Iterator { m_buckets.span(), bucket_index, bucket.before_begin() };
            }
        } else {
            if (it == bucket.end()) {
                ++m_size;
                bucket.push_front(node);
                Tag::did_insert(down_cast_self(), static_cast<ConcreteNode&>(node));
                return vocab::Tuple(Iterator { m_buckets.span(), bucket_index, bucket.before_begin() }, true);
            } else {
                return vocab::Tuple(Iterator { m_buckets.span(), bucket_index, before_it }, false);
            }
        }
    }

    template<typename U>
    u64 hash(U const& value) const {
        auto hasher = m_hasher;
        return container::hash(hasher, value);
    }

    template<typename T, typename U>
    constexpr bool equal(T const& a, U const& b) const {
        return m_eq(a, b);
    }

    Buckets m_buckets {};
    usize m_size { 0 };
    [[no_unique_address]] Eq m_eq {};
    [[no_unique_address]] Hasher m_hasher {};
};
}
