#pragma once

#include <liim/container/hash/table.h>
#include <liim/format.h>
#include <liim/tuple.h>

namespace LIIM::Container::Hash {
template<typename K, typename V>
class Map : public Detail::Table<Tuple<const K, V>, Map<K, V>, Detail::TableType::Map> {
    using Table = Detail::Table<Tuple<const K, V>, Map<K, V>, Detail::TableType::Map>;

public:
    using ValueType = Table::ValueType;
    using Iterator = Table::Iterator;
    using ConstIterator = Table::ConstIterator;

    template<Detail::CanInsertIntoSet<K> U>
    constexpr decltype(auto) operator[](U&& needle);

    template<Detail::CanInsertIntoSet<K> U, typename W>
    constexpr auto insert_or_assign(U&& needle, W&& value) requires(AssignableFrom<V, W> || FalliblyAssignableFrom<V, W>);

    template<Detail::CanInsertIntoSet<K> U, typename... Args>
    constexpr auto try_emplace(U&& needle, Args&&... args) requires(CreateableFrom<V, Args...> || FalliblyCreateableFrom<V, Args...>);

    constexpr auto values();
    constexpr auto values() const;
    constexpr auto cvalues() const { return values(); }
    constexpr auto keys() const;

    constexpr bool operator==(const Map& other) const requires(EqualComparable<V>);
};

template<typename K, typename V>
template<Detail::CanInsertIntoSet<K> U>
constexpr decltype(auto) Map<K, V>::operator[](U&& needle) {
    ValueType* value_location = nullptr;
    return result_and_then(Table::insert_with_factory(forward<U>(needle),
                                                      [&](auto* pointer) {
                                                          value_location = pointer;
                                                          return create_at(pointer, forward<U>(needle), V());
                                                      }),
                           [&](auto&& optional_value) -> V& {
                               if (optional_value) {
                                   return *optional_value;
                               } else {
                                   return tuple_get<1>(*value_location);
                               }
                           });
}

template<typename K, typename V>
template<Detail::CanInsertIntoSet<K> U, typename W>
constexpr auto Map<K, V>::insert_or_assign(U&& needle, W&& value) requires(AssignableFrom<V, W> || FalliblyAssignableFrom<V, W>) {
    return result_and_then(try_emplace(forward<U>(needle), forward<W>(value)), [&](auto&& optional_value_reference) {
        return result_option_and_then(move(optional_value_reference), [&](V& value_reference) {
            auto result = V(move(value_reference));
            return result_and_then(assign_to(value_reference, forward<W>(value)), [&](auto&&) -> V {
                return move(result);
            });
        });
    });
}

template<typename K, typename V>
template<Detail::CanInsertIntoSet<K> U, typename... Args>
constexpr auto Map<K, V>::try_emplace(U&& needle,
                                      Args&&... args) requires(CreateableFrom<V, Args...> || FalliblyCreateableFrom<V, Args...>) {
    return Table::insert_with_factory(forward<U>(needle), [&](ValueType* pointer) {
        return create_at(pointer, piecewise_construct, forward_as_tuple<U>(forward<U>(needle)),
                         forward_as_tuple<Args...>(forward<Args>(args)...));
    });
}

template<typename K, typename V>
constexpr auto Map<K, V>::values() {
    return transform(*this, [](auto& pair) -> V& {
        return tuple_get<1>(pair);
    });
}

template<typename K, typename V>
constexpr auto Map<K, V>::values() const {
    return transform(*this, [](const auto& pair) -> const V& {
        return tuple_get<1>(pair);
    });
}

template<typename K, typename V>
constexpr auto Map<K, V>::keys() const {
    return transform(*this, [](const auto& pair) -> const K& {
        return tuple_get<0>(pair);
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

template<PairLike ValueType>
constexpr auto make_hash_map(std::initializer_list<ValueType> list) {
    using MapType = Map<const DecayTupleElement<0, ValueType>, DecayTupleElement<1, ValueType>>;
    return MapType::create(list.begin(), list.end());
}

template<Iterator Iter>
constexpr auto make_hash_map(Iter start, Iter end, Option<size_t> known_size = {}) {
    using ValueType = decay_t<IteratorValueType<Iter>>;
    using MapType = Map<const DecayTupleElement<0, ValueType>, DecayTupleElement<1, ValueType>>;
    return MapType::create(move(start), move(end), known_size);
}

template<Container C>
constexpr auto collect_hash_map(C&& container) {
    using ValueType = decay_t<ContainerValueType<C>>;
    using MapType = Map<const DecayTupleElement<0, ValueType>, DecayTupleElement<1, ValueType>>;
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
