#pragma once

#include <liim/container/hash/table.h>
#include <liim/format.h>
#include <liim/pair.h>

namespace LIIM::Container::Hash {
template<typename K, typename V>
class Map : public Detail::Table<Pair<const K, V>, Map<K, V>, Detail::TableType::Map> {
    using Table = Detail::Table<Pair<const K, V>, Map<K, V>, Detail::TableType::Map>;

public:
    using ValueType = Table::ValueType;
    using Iterator = Table::Iterator;
    using ConstIterator = Table::ConstIterator;

    template<Detail::CanInsertIntoSet<K> U>
    constexpr V& operator[](U&& needle);

    template<Detail::CanInsertIntoSet<K> U, typename W>
    constexpr Option<V> insert_or_assign(U&& needle, W&& value) requires(AssignableFrom<V, W>);

    template<Detail::CanInsertIntoSet<K> U, typename... Args>
    constexpr Option<V&> try_emplace(U&& needle, Args&&... args) requires(CreateableFrom<V, Args...>);

    constexpr auto values();
    constexpr auto values() const;
    constexpr auto cvalues() const { return values(); }
    constexpr auto keys() const;

    constexpr bool operator==(const Map& other) const requires(EqualComparable<V>);
};

template<typename K, typename V>
template<Detail::CanInsertIntoSet<K> U>
constexpr V& Map<K, V>::operator[](U&& needle) {
    ValueType* value_location = nullptr;
    return Table::insert_with_factory(forward<U>(needle),
                                      [&](auto* pointer) {
                                          value_location = pointer;
                                          create_at(pointer, forward<U>(needle), V());
                                      })
        .value_or(value_location->second);
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
template<Detail::CanInsertIntoSet<K> U, typename... Args>
constexpr Option<V&> Map<K, V>::try_emplace(U&& needle, Args&&... args) requires(CreateableFrom<V, Args...>) {
    return Table::insert_with_factory(forward<U>(needle), [&](ValueType* pointer) {
        create_at(pointer, piecewise_construct, forward_as_tuple<U>(forward<U>(needle)), forward_as_tuple<Args...>(forward<Args>(args)...));
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
