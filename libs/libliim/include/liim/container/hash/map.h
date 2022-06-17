#pragma once

#include <liim/container/hash/table.h>
#include <liim/format.h>
#include <liim/pair.h>

namespace LIIM::Container::Hash {
template<typename K, typename V>
class Map {
public:
    using ValueType = Pair<const K, V>;

private:
    using Table = Detail::Table<ValueType, Detail::TableType::Map>;

public:
    constexpr Map() = default;
    constexpr Map(Map&&) = default;

    static constexpr Map create(std::initializer_list<ValueType> list);
    template<Iterator Iter>
    static constexpr Map create(Iter begin, Iter end, Option<size_t> known_size = {});

    constexpr Map clone() const requires(Cloneable<ValueType>);

    constexpr Map& operator=(Map&&) = default;
    constexpr Map& assign(std::initializer_list<ValueType> list);

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

    template<Detail::CanInsertIntoSet<K> U>
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

    template<typename P>
    constexpr Iterator insert(ConstIterator hint, P&& pair);

    constexpr void insert(std::initializer_list<ValueType> list);

    template<::Iterator Iter>
    constexpr void insert(Iter begin, Iter end, Option<size_t> known_size = {});

    template<::Iterator Iter>
    constexpr Iterator insert(ConstIterator hint, Iter begin, Iter end, Option<size_t> known_size = {});

    template<Detail::CanInsertIntoSet<K> U, typename W>
    constexpr Option<V> insert_or_assign(U&& needle, W&& value) requires(AssignableFrom<V, W>);

    template<typename... Args>
    constexpr Option<V&> emplace(Args&&... args) requires(CreateableFrom<ValueType, Args...>);

    template<Detail::CanInsertIntoSet<K> U, typename... Args>
    constexpr Option<V&> try_emplace(U&& needle, Args&&... args) requires(CreateableFrom<V, Args...>);

    constexpr auto values();
    constexpr auto values() const;
    constexpr auto cvalues() const { return values(); }
    constexpr auto keys() const;

    constexpr void swap(Map& other) { m_table.swap(other.m_table); }

    constexpr bool operator==(const Map& other) const requires(EqualComparable<V>);

private:
    constexpr Map(Table&& table) : m_table(move(table)) {}

    Table m_table;
};

template<typename K, typename V>
constexpr auto Map<K, V>::create(std::initializer_list<ValueType> list) -> Map {
    return Map::create(list.begin(), list.end());
}

template<typename K, typename V>
template<Iterator Iter>
constexpr auto Map<K, V>::create(Iter begin, Iter end, Option<size_t> known_size) -> Map {
    auto result = Map {};
    result.insert(move(begin), move(end), known_size);
    return result;
}

template<typename K, typename V>
constexpr auto Map<K, V>::assign(std::initializer_list<ValueType> list) -> Map& {
    return *this = Map::create(list);
}

template<typename K, typename V>
constexpr auto Map<K, V>::clone() const -> Map requires(Cloneable<ValueType>) {
    return Map(m_table.clone());
}

template<typename K, typename V>
template<Detail::CanInsertIntoSet<K> U>
constexpr V& Map<K, V>::operator[](U&& needle) {
    ValueType* value_location = nullptr;
    return m_table
        .insert_with_factory(forward<U>(needle),
                             [&](auto* pointer) {
                                 value_location = pointer;
                                 create_at(pointer, forward<U>(needle), V());
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
template<typename P>
constexpr auto Map<K, V>::insert(ConstIterator hint, P&& pair) -> Iterator {
    return m_table.insert(hint, forward<P>(pair));
}

template<typename K, typename V>
constexpr void Map<K, V>::insert(std::initializer_list<ValueType> list) {
    insert(list.begin(), list.end());
}

template<typename K, typename V>
template<::Iterator Iter>
constexpr void Map<K, V>::insert(Iter begin, Iter end, Option<size_t> known_size) {
    return m_table.insert(move(begin), move(end), known_size);
}

template<typename K, typename V>
template<::Iterator Iter>
constexpr auto Map<K, V>::insert(ConstIterator hint, Iter begin, Iter end, Option<size_t> known_size) -> Iterator {
    return m_table.insert(hint, move(begin), move(end), known_size);
}

template<typename K, typename V>
template<Detail::CanInsertIntoSet<K> U, typename W>
constexpr Option<V> Map<K, V>::insert_or_assign(U&& needle, W&& value) requires(AssignableFrom<V, W>) {
    return try_emplace(forward<U>(needle), forward<W>(value)).map([&](auto& value_reference) {
        auto result = V(move(value_reference));
        assign_to(value_reference, forward<W>(value));
        return result;
    });
}

template<typename K, typename V>
template<typename... Args>
constexpr Option<V&> Map<K, V>::emplace(Args&&... args) requires(CreateableFrom<ValueType, Args...>) {
    return insert(create<ValueType>(forward<Args>(args)...));
}

template<typename K, typename V>
template<Detail::CanInsertIntoSet<K> U, typename... Args>
constexpr Option<V&> Map<K, V>::try_emplace(U&& needle, Args&&... args) requires(CreateableFrom<V, Args...>) {
    return m_table
        .insert_with_factory(forward<U>(needle),
                             [&](ValueType* pointer) {
                                 create_at(pointer, piecewise_construct, forward_as_tuple<U>(forward<U>(needle)),
                                           forward_as_tuple<Args...>(forward<Args>(args)...));
                             })
        .map([](auto& value) -> V& {
            return value.second;
        });
}

template<typename K, typename V>
constexpr auto Map<K, V>::values() {
    return transform(*this, [](auto& pair) -> V& {
        return pair.second;
    });
}

template<typename K, typename V>
constexpr auto Map<K, V>::values() const {
    return transform(*this, [](const auto& pair) -> const V& {
        return pair.second;
    });
}

template<typename K, typename V>
constexpr auto Map<K, V>::keys() const {
    return transform(*this, [](const auto& pair) -> const K& {
        return pair.first;
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

template<typename ValueType>
constexpr auto make_hash_map(std::initializer_list<ValueType> list) {
    using MapType = Map<const typename ValueType::FirstType, typename ValueType::SecondType>;
    return MapType::create(list.begin(), list.end());
}

template<Iterator Iter>
constexpr auto make_hash_map(Iter start, Iter end, Option<size_t> known_size = {}) {
    using ValueType = decay_t<IteratorValueType<Iter>>;
    using MapType = Map<const typename ValueType::FirstType, typename ValueType::SecondType>;
    return MapType::create(move(start), move(end), known_size);
}

template<Container C>
constexpr auto collect_hash_map(C&& container) {
    using ValueType = decay_t<ContainerValueType<C>>;
    using MapType = Map<decay_t<typename ValueType::FirstType>, decay_t<typename ValueType::SecondType>>;
    return collect<MapType>(forward<C>(container));
}
}

namespace LIIM::Format {
template<Formattable K, Formattable V>
struct Formatter<LIIM::Container::Hash::Map<K, V>> {
    constexpr void parse(FormatParseContext&) {}

    void format(const LIIM::Container::Hash::Map<K, V>& map, FormatContext& context) {
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
