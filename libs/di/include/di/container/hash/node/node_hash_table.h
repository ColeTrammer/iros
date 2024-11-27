#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/algorithm/find_if.h>
#include <di/container/algorithm/uninitialized_default_construct.h>
#include <di/container/hash/hash.h>
#include <di/container/hash/hash_same.h>
#include <di/container/hash/hasher.h>
#include <di/container/hash/node/hash_node.h>
#include <di/container/hash/node/hash_node_iterator.h>
#include <di/container/vector/mutable_vector.h>
#include <di/function/monad/monad_try.h>
#include <di/function/not_fn.h>
#include <di/meta/algorithm.h>
#include <di/meta/relation.h>
#include <di/meta/util.h>
#include <di/util/declval.h>
#include <di/util/get.h>
#include <di/vocab/expected/prelude.h>
#include <di/vocab/tuple/tuple_element.h>

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

    template<typename Key, typename Value, typename Eq>
    struct NodeHashTableMapValidForLookup {
        template<typename U>
        struct Type {
            constexpr static inline bool value =
                (concepts::Predicate<Eq&, Key const&, U const&> && concepts::HashSame<Key, U>) ||
                (concepts::RemoveCVRefSameAs<U, Tuple<Key, Value>>);
        };
    };

    template<typename Value, bool is_map>
    struct NodeHashTableKey {
        using Type = Value;
    };

    template<typename Value>
    struct NodeHashTableKey<Value, true> {
        using Type = meta::Front<meta::AsList<Value>>;
    };
}

/// @brief Node based (closed addressing) hash table.
///
/// This is fairly straightforward implementation of a hash table. It uses a vector of buckets, where each bucket is a
/// singlely-linked list of nodes.
template<typename Value, typename Eq, concepts::Hasher Hasher, typename Buckets, typename Tag, typename Interface,
         bool is_multi, bool is_map, typename Self = Void>
class NodeHashTable : public Interface {
private:
    template<typename, typename, concepts::Hasher, typename, typename, typename, bool, bool, typename>
    friend class NodeHashTable;

protected:
    using Node = HashNode<Tag>;
    using Iterator = HashNodeIterator<Value, Tag>;
    using ConstIterator = container::ConstIteratorImpl<Iterator>;

    using ConcreteNode = decltype(Tag::node_type(in_place_type<Value>));

    using Key = meta::Type<detail::NodeHashTableKey<Value, is_map>>;

    constexpr auto down_cast_self() -> decltype(auto) {
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
    auto operator=(NodeHashTable const&) -> NodeHashTable& = delete;

    constexpr explicit NodeHashTable(Eq eq, Hasher hasher = {}) : m_eq(util::move(eq)), m_hasher(util::move(hasher)) {}

    constexpr NodeHashTable(NodeHashTable&& other)
        : m_buckets(util::move(other.m_buckets))
        , m_size(util::exchange(other.m_size, 0))
        , m_eq(util::move(other.m_eq))
        , m_hasher(util::move(other.m_hasher)) {}

    constexpr auto operator=(NodeHashTable&& other) -> NodeHashTable& {
        this->clear();
        m_buckets = util::move(other.m_buckets);
        m_size = util::exchange(other.m_size, 0);
        m_eq = util::move(other.m_eq);
        m_hasher = util::move(other.m_hasher);
        return *this;
    }

    constexpr ~NodeHashTable() { this->clear(); }

    constexpr auto size() const -> usize { return m_size; }
    constexpr auto empty() const -> bool { return m_size == 0; }
    constexpr auto bucket_count() const -> usize { return vector::size(m_buckets); }

    constexpr auto begin() -> Iterator { return unconst_iterator(util::as_const(*this).begin()); }
    constexpr auto begin() const -> ConstIterator {
        auto const buckets = m_buckets.span();
        auto it = container::find_if(m_buckets, function::not_fn(container::empty));
        if (it == m_buckets.end()) {
            return {};
        }
        auto const bucket_index = it - buckets.begin();
        return Iterator(buckets, bucket_index);
    }
    constexpr auto end() -> Iterator { return unconst_iterator(util::as_const(*this).end()); }
    constexpr auto end() const -> ConstIterator { return Iterator(m_buckets.span(), m_buckets.size()); }

    constexpr auto unconst_iterator(ConstIterator it) -> Iterator { return it.base(); }

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

    constexpr auto erase_impl(ConstIterator it) -> Iterator {
        auto bucket_index = it.base().bucket_index();
        auto& bucket = m_buckets[bucket_index];
        auto& node = static_cast<ConcreteNode&>(it.base().node());
        auto const next = bucket.erase_after(it.base().before_current());
        --m_size;

        Tag::did_remove(down_cast_self(), node);
        if (next == bucket.end()) {
            for (++bucket_index; bucket_index < bucket_count(); ++bucket_index) {
                if (!vector::lookup(m_buckets, bucket_index).empty()) {
                    return Iterator(m_buckets.span(), bucket_index);
                }
            }
            return end();
        }
        return Iterator { m_buckets.span(), bucket_index, it.base().before_current() };
    }

    template<typename U>
    requires(concepts::Predicate<Eq&, Key const&, U const&> && concepts::HashSame<Key, U>)
    constexpr auto equal_range_impl(U&& needle) const {
        if (empty()) {
            return View<ConstIterator> { end(), end() };
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
            return View<ConstIterator> { end(), end() };
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
        return View<ConstIterator> { first, last };
    }

    template<typename U>
    requires(concepts::Predicate<Eq&, Key const&, U const&> && concepts::HashSame<Key, U>)
    constexpr auto find_impl(U&& needle) const -> ConstIterator {
        if (empty()) {
            return end();
        }

        auto const hash = this->hash(needle);
        auto const bucket_index = hash % m_buckets.size();
        auto const& bucket = m_buckets[bucket_index];
        auto before_it = bucket.before_begin();
        while (container::next(before_it) != bucket.end()) {
            auto&& current = *container::next(before_it);
            if (this->equal(this->node_value(current), needle)) {
                return Iterator(m_buckets.span(), bucket_index, before_it.base());
            }
            ++before_it;
        }
        return end();
    }

    constexpr auto reserve(usize new_capacity)
        -> decltype(util::declval<Buckets&>().reserve_from_nothing(new_capacity)) {
        if (vector::empty(m_buckets)) {
            if constexpr (concepts::LanguageVoid<decltype(util::declval<Buckets&>().reserve_from_nothing(
                              new_capacity))>) {
                m_buckets.reserve_from_nothing(new_capacity);
            } else {
                DI_TRY(m_buckets.reserve_from_nothing(new_capacity));
            }
            m_buckets.assume_size(new_capacity);
            container::uninitialized_default_construct(m_buckets.span());
            if constexpr (concepts::LanguageVoid<decltype(util::declval<Buckets&>().reserve_from_nothing(
                              new_capacity))>) {
                return;
            } else {
                return {};
            }
        }

        if (new_capacity <= m_buckets.size()) {
            return;
        }

        auto new_buckets = Buckets {};
        if constexpr (concepts::LanguageVoid<decltype(util::declval<Buckets&>().reserve_from_nothing(new_capacity))>) {
            new_buckets.reserve_from_nothing(new_capacity);
        } else {
            DI_TRY(new_buckets.reserve_from_nothing(new_capacity));
        }
        new_buckets.assume_size(new_capacity);
        m_size = 0;
        container::uninitialized_default_construct(new_buckets.span());

        auto old_buckets = util::move(m_buckets);
        m_buckets = util::move(new_buckets);
        for (auto& bucket : old_buckets) {
            while (!bucket.empty()) {
                auto& node = static_cast<Node&>(*bucket.begin().node());
                bucket.pop_front();
                insert_node_without_rehashing(node, false);
            }
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
    constexpr auto node_value(Node& node) const -> Value& {
        return Tag::down_cast(in_place_type<Value>, static_cast<ConcreteNode&>(node));
    }
    constexpr auto node_value(Node const& node) const -> Value const& {
        return const_cast<NodeHashTable&>(*this).node_value(const_cast<Node&>(node));
    }

    constexpr void merge_impl_without_rehashing(NodeHashTable&& other) {
        auto it = other.begin();
        auto const end = other.end();
        for (; it != end;) {
            auto& node = it.node();
            ++it;
            this->insert_node_without_rehashing(node, false);
        }
    }

    constexpr auto insert_node_without_rehashing(Node& node, bool call_insertion_hook = true) {
        DI_ASSERT(!vector::empty(m_buckets));

        auto const hash = this->hash(node_value(node));
        auto const bucket_index = hash % vector::size(m_buckets);
        auto& bucket = vector::lookup(m_buckets, bucket_index);

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
                if (call_insertion_hook) {
                    Tag::did_insert(down_cast_self(), static_cast<ConcreteNode&>(node));
                }
                return Iterator { m_buckets.span(), bucket_index, it };
            } else {
                bucket.push_front(node);
                if (call_insertion_hook) {
                    Tag::did_insert(down_cast_self(), static_cast<ConcreteNode&>(node));
                }
                return Iterator { m_buckets.span(), bucket_index, bucket.before_begin() };
            }
        } else {
            if (it == bucket.end()) {
                ++m_size;
                bucket.push_front(node);
                if (call_insertion_hook) {
                    Tag::did_insert(down_cast_self(), static_cast<ConcreteNode&>(node));
                }
                return vocab::Tuple(Iterator { m_buckets.span(), bucket_index, bucket.before_begin() }, true);
            } else {
                return vocab::Tuple(Iterator { m_buckets.span(), bucket_index, before_it }, false);
            }
        }
    }

    template<typename U>
    constexpr auto hash(U const& value) const -> u64 {
        auto hasher = m_hasher;
        return container::hash(hasher, value);
    }

    constexpr auto hash(Value const& value) const -> u64 {
        auto hasher = m_hasher;
        if constexpr (is_map) {
            return container::hash(hasher, util::get<0>(value));
        } else {
            return container::hash(hasher, value);
        }
    }

    template<typename T, typename U>
    constexpr auto equal(T const& a, U const& b) const -> bool {
        return m_eq(a, b);
    }

    template<typename T>
    constexpr auto equal(T const& a, Value const& b) const -> bool {
        if constexpr (is_map) {
            return m_eq(a, util::get<0>(b));
        } else {
            return m_eq(a, b);
        }
    }

    template<typename T>
    constexpr auto equal(Value const& a, T const& b) const -> bool {
        if constexpr (is_map) {
            return m_eq(util::get<0>(a), b);
        } else {
            return m_eq(a, b);
        }
    }

    constexpr auto equal(Value const& a, Value const& b) const -> bool {
        if constexpr (is_map) {
            return m_eq(util::get<0>(a), util::get<0>(b));
        } else {
            return m_eq(a, b);
        }
    }

    Buckets m_buckets {};
    usize m_size { 0 };
    [[no_unique_address]] Eq m_eq {};
    [[no_unique_address]] Hasher m_hasher {};
};
}
