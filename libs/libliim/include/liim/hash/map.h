#pragma once

#include <liim/format.h>
#include <liim/hash/table.h>
#include <liim/pair.h>

namespace LIIM::Hash {
template<typename K, typename V>
class Map {
public:
    using ValueType = Pair<const K, V>;

private:
    using Table = Detail::Table<ValueType, Detail::TableType::Map>;

public:
    constexpr Map() = default;
    constexpr Map(const Map&) = default;
    constexpr Map(Map&&) = default;
    constexpr Map(std::initializer_list<ValueType> list) { insert(list); }
    template<Iterator Iter>
    constexpr Map(Iter begin, Iter end) : Map(iterator_container(::move(begin), ::move(end))) {}
    template<Container C>
    constexpr Map(C&& container) {
        insert(::forward<C>(container));
    }

    constexpr Map& operator=(const Map&) = default;
    constexpr Map& operator=(Map&&) = default;
    constexpr Map& operator=(std::initializer_list<ValueType> list);

    constexpr bool empty() const { return m_table.empty(); }
    constexpr size_t size() const { return m_table.size(); }

    using Iterator = Table::Iterator;
    using ConstIterator = Table::ConstIterator;

    constexpr auto begin() { return m_table.begin(); }
    constexpr auto begin() const { return m_table.begin(); }
    constexpr auto cbegin() const { return m_table.cbegin(); }

    constexpr auto end() { return m_table.end(); }
    constexpr auto end() const { return m_table.end(); }
    constexpr auto cend() const { return m_table.cend(); }

    constexpr void clear() { m_table.clear(); }

    template<Detail::CanInsert<K> U>
    constexpr V& operator[](U&& needle);

    template<Detail::CanLookup<K> U>
    constexpr Option<V&> at(U&& needle);

    template<Detail::CanLookup<K> U>
    constexpr Option<const V&> at(U&& needle) const;

    template<Detail::CanLookup<K> U>
    constexpr Iterator find(U&& needle);

    template<Detail::CanLookup<K> U>
    constexpr ConstIterator find(U&& needle) const;

    template<Detail::CanLookup<K> U>
    constexpr bool contains(U&& needle) const;

    template<Detail::CanLookup<K> U>
    constexpr Option<V> erase(U&& needle);

    constexpr Option<V> erase(ConstIterator position);
    constexpr void erase(ConstIterator start, ConstIterator end);

    template<typename P>
    constexpr Option<V&> insert(P&& pair);

    constexpr void insert(std::initializer_list<ValueType> list);

    template<::Iterator Iter>
    constexpr void insert(Iter begin, Iter end) {
        insert(iterator_container(move(begin), move(end)));
    }

    template<Container C>
    constexpr void insert(C&& container);

    template<Detail::CanInsert<K> U, typename W>
    constexpr Option<V> insert_or_assign(U&& needle, W&& value);

    template<typename... Args>
    constexpr Option<V&> emplace(Args&&... args);

    template<Detail::CanInsert<K> U, typename... Args>
    constexpr Option<V&> try_emplace(U&& needle, Args&&... args);

    constexpr void swap(Map& other) { m_table.swap(other.m_table); }

    constexpr bool operator==(const Map& other) const requires(EqualComparable<V>);

private:
    Table m_table;
};

template<typename K, typename V>
constexpr auto Map<K, V>::operator=(std::initializer_list<ValueType> list) -> Map& {
    auto new_map = Map(list);
    swap(new_map);
    return *this;
}

template<typename K, typename V>
template<Detail::CanInsert<K> U>
constexpr V& Map<K, V>::operator[](U&& needle) {
    ValueType* value_location = nullptr;
    return m_table
        .insert_with_factory(forward<U>(needle),
                             [&](auto* pointer) {
                                 value_location = pointer;
                                 construct_at(pointer, forward<U>(needle), V());
                             })
        .map([](auto& value) -> V& {
            return value.second;
        })
        .value_or(value_location->second);
}

template<typename K, typename V>
template<Detail::CanLookup<K> U>
constexpr Option<V&> Map<K, V>::at(U&& needle) {
    return m_table.at(forward<U>(needle)).map([](auto& value) -> V& {
        return value.second;
    });
}

template<typename K, typename V>
template<Detail::CanLookup<K> U>
constexpr Option<const V&> Map<K, V>::at(U&& needle) const {
    return m_table.at(forward<U>(needle)).map([](const auto& value) -> const V& {
        return value.second;
    });
}

template<typename K, typename V>
template<Detail::CanLookup<K> U>
constexpr auto Map<K, V>::find(U&& needle) -> Iterator {
    return m_table.find(forward<U>(needle));
}

template<typename K, typename V>
template<Detail::CanLookup<K> U>
constexpr auto Map<K, V>::find(U&& needle) const -> ConstIterator {
    return m_table.find(forward<U>(needle));
}

template<typename K, typename V>
template<Detail::CanLookup<K> U>
constexpr bool Map<K, V>::contains(U&& needle) const {
    return !!at(forward<U>(needle));
}

template<typename K, typename V>
template<Detail::CanLookup<K> U>
constexpr Option<V> Map<K, V>::erase(U&& needle) {
    return m_table.erase(forward<U>(needle)).map([](auto&& value) -> V {
        return move(value).second;
    });
}

template<typename K, typename V>
constexpr Option<V> Map<K, V>::erase(ConstIterator position) {
    return m_table.erase(position).map([](auto&& value) -> V {
        return move(value).second;
    });
}

template<typename K, typename V>
constexpr void Map<K, V>::erase(ConstIterator start, ConstIterator end) {
    for (auto it = start; it != end; ++it) {
        erase(it);
    }
}

template<typename K, typename V>
template<typename P>
constexpr Option<V&> Map<K, V>::insert(P&& pair) {
    return m_table.insert(forward<P>(pair)).map([](auto& value) -> V& {
        return value.second;
    });
}

template<typename K, typename V>
constexpr void Map<K, V>::insert(std::initializer_list<ValueType> list) {
    insert(list.begin(), list.end());
}

template<typename K, typename V>
template<Container C>
constexpr void Map<K, V>::insert(C&& container) {
    using ValueType = IteratorTraits<decltype(container.begin())>::ValueType;
    constexpr bool is_const = IsConst<typename RemoveReference<ValueType>::type>::value;

    if constexpr (!is_const && !IsLValueReference<C>::value && !IsConst<decay_t<C>>::value) {
        for (auto&& value : move_elements(::move(container))) {
            insert(move(value));
        }
    } else {
        for (const auto& value : container) {
            insert(value);
        }
    }
}

template<typename K, typename V>
template<Detail::CanInsert<K> U, typename W>
constexpr Option<V> Map<K, V>::insert_or_assign(U&& needle, W&& value) {
    return try_emplace(forward<U>(needle), forward<W>(value)).map([&](auto& value_reference) {
        auto result = V(move(value_reference));
        value_reference = forward<W>(value);
        return result;
    });
}

template<typename K, typename V>
template<typename... Args>
constexpr Option<V&> Map<K, V>::emplace(Args&&... args) {
    return insert(ValueType(forward<Args>(args)...));
}

template<typename K, typename V>
template<Detail::CanInsert<K> U, typename... Args>
constexpr Option<V&> Map<K, V>::try_emplace(U&& needle, Args&&... args) {
    return m_table
        .insert_with_factory(forward<U>(needle),
                             [&](ValueType* pointer) {
                                 construct_at(pointer, piecewise_construct, forward_as_tuple<U>(forward<U>(needle)),
                                              forward_as_tuple<Args...>(forward<Args>(args)...));
                             })
        .map([](auto& value) -> V& {
            return value.second;
        });
}

template<typename K, typename V>
constexpr bool Map<K, V>::operator==(const Map& other) const requires(EqualComparable<V>) {
    if (this->size() != other.size()) {
        return false;
    }
    for (auto& [key, value] : *this) {
        if (!other.at(key)
                 .map([&](const auto& other_value) -> bool {
                     return value == other_value;
                 })
                 .value_or(false)) {
            return false;
        }
    }
    return true;
}

template<typename K, typename V>
Map(std::initializer_list<Pair<K, V>>) -> Map<K, V>;
}

namespace LIIM::Format {
template<Formattable K, Formattable V>
struct Formatter<LIIM::Hash::Map<K, V>> {
    constexpr void parse(FormatParseContext&) {}

    void format(const LIIM::Hash::Map<K, V>& map, FormatContext& context) {
        context.put("{ ");
        bool first = true;
        for (auto& [key, value] : map) {
            if (!first) {
                context.put(", ");
            }
            format_to_context(context, "{}: {}", key, value);
            first = false;
        }
        context.put(" }");
    }
};
}
