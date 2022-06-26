#pragma once

// Hash Table implementation modelled from https://www.youtube.com/watch?v=ncHmEUmJZf4.

#include <liim/compare.h>
#include <liim/container/container.h>
#include <liim/container/hash/entry.h>
#include <liim/container/hash/group_info.h>
#include <liim/container/hash/hashable.h>
#include <liim/container/hash/hasher.h>
#include <liim/container/hash/table_iterator.h>
#include <liim/container/new_vector.h>
#include <liim/result.h>

namespace LIIM::Container::Hash::Detail {
template<typename TransparentKey, typename Base>
concept CanLookup = HashableLike<TransparentKey, Base> && EqualComparableWith<TransparentKey, Base>;

template<typename TransparentKey, typename KeyType>
concept CanInsertIntoSet = (CanLookup<TransparentKey, KeyType> &&
                                (CreateableFrom<KeyType, TransparentKey> || FalliblyCreateableFrom<KeyType, TransparentKey>) ||
                            FalliblyCreateableFrom<KeyType, TransparentKey>);

template<typename Pair, typename KeyType, typename StorageType>
concept CanInsertIntoMap = (CanLookup<typename decay_t<Pair>::FirstType, KeyType> &&
                                (CreateableFrom<StorageType, Pair> || FalliblyCreateableFrom<StorageType, Pair>) ||
                            FalliblyCreateableFrom<StorageType, Pair>);

template<typename U, typename Table>
concept CanInsert = (Table::is_set && CanInsertIntoSet<U, typename Table::ValueType>) ||
                    (Table::is_map && CanInsertIntoMap<U, typename Table::KeyType, typename Table::ValueType>);

enum class TableType {
    Set,
    Map,
};

template<typename T, TableType table_type>
struct TableKeyType {
    using Type = T;
};

template<typename T>
struct TableKeyType<T, TableType::Map> {
    using Type = RemoveConst<typename T::FirstType>::type;
};

template<typename T, TableType table_type>
struct TableValue {
    using Type = T;
};

template<typename T>
struct TableValue<T, TableType::Map> {
    using Type = T::SecondType;
};

template<typename T, typename Self, TableType type>
class Table {
private:
    constexpr static bool is_set = type == TableType::Set;
    constexpr static bool is_map = type == TableType::Map;

public:
    using ValueType = T;
    using KeyType = TableKeyType<T, type>::Type;
    using Value = TableValue<T, type>::Type;
    using Iterator = TableIterator<Table>;
    using ConstIterator = TableIterator<const Table>;
    using AllocatorResult = Void;

    constexpr static auto create(std::initializer_list<T> list) { return create(list.begin(), list.end()); }

    template<::Iterator Iter>
    constexpr static auto create(Iter start, Iter end, Option<size_t> known_size = {}) {
        auto result = Self {};
        return result_and_then(result.insert(move(start), move(end), known_size), [&](auto&&) -> Self {
            return move(result);
        });
    }

    constexpr Table() {}
    constexpr Table(Table&&);
    constexpr ~Table();

    constexpr Table& operator=(Table&&);

    constexpr decltype(auto) assign(std::initializer_list<T> list);

    constexpr auto clone() const requires(Cloneable<ValueType>);

    constexpr bool empty() const { return size() == 0; }
    constexpr size_t size() const { return m_size; }

    friend Iterator;
    friend ConstIterator;

    constexpr Iterator begin() { return Iterator(*this); }
    constexpr ConstIterator begin() const { return ConstIterator(*this); }
    constexpr ConstIterator cbegin() const { return ConstIterator(*this); }

    constexpr Iterator end() { return Iterator(*this, m_capacity, 0); }
    constexpr ConstIterator end() const { return ConstIterator(*this, m_capacity, 0); }
    constexpr ConstIterator cend() const { return ConstIterator(*this, m_capacity, 0); }

    constexpr void clear();

    template<typename U, typename Factory>
    constexpr CommonResult<Option<Value&>, AllocatorResult, typename InvokeResult<Factory, T*>::type>
    insert_with_factory(U&& needle, Factory&& factory) requires(CanLookup<U, KeyType>);

    template<typename U>
    constexpr auto insert(U&& to_insert) requires(CanInsert<U, Table>);

    template<typename U>
    constexpr auto insert(ConstIterator hint, U&& to_insert) requires(CanInsert<U, Table>);

    template<::Iterator Iter>
    constexpr CommonResult<Void, AllocatorResult, CreateAtResult<T, IteratorValueType<Iter>>>
    insert(Iter start, Iter end, Option<size_t> known_size = {}) requires(CanInsert<IteratorValueType<Iter>, Table>);

    template<::Iterator Iter>
    constexpr auto insert(ConstIterator hint, Iter start, Iter end,
                          Option<size_t> known_size = {}) requires(CanInsert<IteratorValueType<Iter>, Table>);

    constexpr auto insert(std::initializer_list<T> list) { return insert(list.begin(), list.end()); }
    constexpr auto insert(ConstIterator hint, std::initializer_list<T> list) { return insert(hint, list.begin(), list.end()); }

    template<typename... Args>
    constexpr auto emplace(Args&&... args) requires(CreateableFrom<ValueType, Args...>);

    template<typename U>
    constexpr Option<Value&> at(U&& needle) requires(CanLookup<U, KeyType>);

    template<typename U>
    constexpr Option<const Value&> at(U&& needle) const requires(CanLookup<U, KeyType>);

    template<typename U>
    constexpr Iterator find(U&& needle) requires(CanLookup<U, KeyType>);

    template<typename U>
    constexpr ConstIterator find(U&& needle) const requires(CanLookup<U, KeyType>);

    template<typename U>
    constexpr bool contains(U&& needle) const requires(CanLookup<U, KeyType>);

    constexpr Option<Value> erase(ConstIterator iterator);

    constexpr void erase(ConstIterator start, ConstIterator end);

    template<typename U>
    constexpr Option<Value> erase(U&& needle) requires(CanLookup<U, KeyType>);

    constexpr void swap(Table& other);

private:
    explicit constexpr Table(size_t capacity);

    enum class FindType {
        PureFind,
        FindToInsert,
    };

    template<FindType find_type, typename U>
    constexpr Option<Entry> find_impl(uint64_t hash_high, uint8_t hash_low, U&& needle) const requires(EqualComparableWith<U, KeyType>);

    constexpr void grow_and_rehash();
    constexpr Option<Value> erase_entry(Entry entry);
    constexpr Entry entry_from_iterator(ConstIterator iterator) const;
    constexpr Iterator iterator_from_entry(Entry entry);
    constexpr ConstIterator iterator_from_entry(Entry entry) const;

    struct HashSplit {
        uint64_t hash_high;
        uint8_t hash_low;
    };

    template<typename U>
    constexpr HashSplit hash(U&& value) const requires(HashableLike<U, KeyType>);

    template<typename U>
    constexpr bool equal(const T& value, U&& other) const requires(EqualComparableWith<U, KeyType>);

    constexpr size_t group_count() const { return m_capacity; }

    constexpr size_t value_index(size_t group_index, uint8_t index_into_group) const {
        return GroupInfo::entry_count * group_index + index_into_group;
    }

    constexpr Value& value(size_t index) { return const_cast<Value&>(const_cast<const Table&>(*this).value(index)); }
    constexpr const Value& value(size_t index) const;

    constexpr T& value_internal(size_t index) { return m_values[index].value; }
    constexpr const T& value_internal(size_t index) const { return m_values[index].value; }

    constexpr T* value_pointer_for_init(size_t index) { return &m_values[index].value; }

    GroupInfo* m_groups { nullptr };
    MaybeUninit<T>* m_values { nullptr };
    size_t m_size { 0 };
    size_t m_used_values { 0 };
    size_t m_capacity { 0 };
};

template<typename T, typename Self, TableType type>
constexpr Table<T, Self, type>::Table(Table&& other)
    : m_groups(exchange(other.m_groups, nullptr))
    , m_values(exchange(other.m_values, nullptr))
    , m_size(exchange(other.m_size, 0))
    , m_used_values(exchange(other.m_used_values, 0))
    , m_capacity(exchange(other.m_capacity, 0)) {}

template<typename T, typename Self, TableType type>
constexpr Table<T, Self, type>::Table(size_t capacity) : m_capacity(capacity) {
    m_groups = new GroupInfo[capacity];
    m_values = new MaybeUninit<T>[capacity * GroupInfo::entry_count];
}

template<typename T, typename Self, TableType type>
constexpr Table<T, Self, type>::~Table() {
    clear();
}

template<typename T, typename Self, TableType type>
constexpr auto Table<T, Self, type>::clone() const requires(Cloneable<T>) {
    return Table::create(begin(), end(), size());
}

template<typename T, typename Self, TableType type>
constexpr auto Table<T, Self, type>::operator=(Table&& other) -> Table& {
    if (this != &other) {
        auto new_table = Table(move(other));
        swap(new_table);
    }
    return *this;
}

template<typename T, typename Self, TableType type>
constexpr decltype(auto) Table<T, Self, type>::assign(std::initializer_list<T> list) {
    return *this = Table::create(list);
}

template<typename T, typename Self, TableType type>
constexpr void Table<T, Self, type>::grow_and_rehash() {
    auto new_capacity = 2 * m_capacity ?: 8;
    auto new_table = Table(new_capacity);
    for (auto& value : *this) {
        new_table.insert(move(value));
    }
    this->swap(new_table);
}

template<typename T, typename Self, TableType type>
constexpr auto Table<T, Self, type>::entry_from_iterator(ConstIterator iterator) const -> Entry {
    return Entry(m_groups[iterator.m_group_index].entry(iterator.m_index_into_group),
                 value_index(iterator.m_group_index, iterator.m_index_into_group));
}

template<typename T, typename Self, TableType type>
constexpr auto Table<T, Self, type>::iterator_from_entry(Entry entry) -> Iterator {
    return Iterator(*this, entry.value_index() / group_count(), entry.value_index() % group_count());
}

template<typename T, typename Self, TableType type>
constexpr auto Table<T, Self, type>::iterator_from_entry(Entry entry) const -> ConstIterator {
    return ConstIterator(*this, entry.value_index() / group_count(), entry.value_index() % group_count());
}

template<typename T, typename Self, TableType type>
constexpr auto Table<T, Self, type>::value(size_t index) const -> const Value& {
    if constexpr (is_map) {
        return value_internal(index).second;
    } else {
        return value_internal(index);
    }
}

template<typename T, typename Self, TableType type>
template<typename U, typename Factory>
constexpr auto Table<T, Self, type>::insert_with_factory(U&& needle, Factory&& factory)
    -> CommonResult<Option<Value&>, AllocatorResult, typename InvokeResult<Factory, T*>::type>
requires(CanLookup<U, KeyType>) {
    auto [hash_high, hash_low] = hash(forward<U>(needle));

    auto entry = find_impl<FindType::FindToInsert>(hash_high, hash_low, forward<U>(needle));
    if (!entry) {
        grow_and_rehash();
        entry = find_impl<FindType::FindToInsert>(hash_high, hash_low, forward<U>(needle));
        assert(entry);
    }

    if (entry->info().present()) {
        return { value(entry->value_index()) };
    }

    return result_and_then(forward<Factory>(factory)(value_pointer_for_init(entry->value_index())), [&](auto&&) -> Option<Value&> {
        entry->info().set_present(hash_low);
        m_size++;
        m_used_values++;
        return None {};
    });
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr auto Table<T, Self, type>::insert(U&& to_insert) requires(CanInsert<U, Table>) {

    if constexpr (is_set) {
        if constexpr (!CanLookup<U, Table>) {
            return result_and_then(LIIM::create<T>(forward<U>(to_insert)), [&](auto&& value) {
                return insert_with_factory(move(value), [&](T* pointer) {
                    return create_at(pointer, move(value));
                });
            });
        } else {
            return insert_with_factory(forward<U>(to_insert), [&](T* pointer) {
                return create_at(pointer, forward<U>(to_insert));
            });
        }
    } else {
        if constexpr (!requires { to_insert.first; }) {
            return result_and_then(LIIM::create<Pair<KeyType, Value>>(forward<U>(to_insert)), [&](auto&& value) {
                return insert_with_factory(move(value).first, [&](T* pointer) {
                    return create_at(pointer, move(value));
                });
            });
        } else {
            return insert_with_factory(forward<U>(to_insert).first, [&](T* pointer) {
                return create_at(pointer, forward<U>(to_insert));
            });
        }
    }
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr auto Table<T, Self, type>::insert(ConstIterator, U&& to_insert) requires(CanInsert<U, Table>) {
    return result_and_then(insert(forward<U>(to_insert)), [&](auto&&) {
        return end();
    });
}

template<typename T, typename Self, TableType type>
template<::Iterator Iter>
constexpr auto Table<T, Self, type>::insert(Iter start, Iter end, Option<size_t> known_size)
    -> CommonResult<Void, AllocatorResult, CreateAtResult<T, IteratorValueType<Iter>>>
requires(CanInsert<IteratorValueType<Iter>, Table>) {
    if constexpr (CreateableFrom<T, IteratorValueType<Iter>>) {
        for (auto it = move(start); it != end; ++it) {
            insert(*it);
        }
        return {};
    } else {
        return result_and_then(collect<NewVector<Pair<KeyType, Value>>>(iterator_container(move(start), move(end))), [&](auto&& elements) {
            auto container = move_elements(move(elements));
            return insert(container.begin(), container.end(), known_size);
        });
    }
}

template<typename T, typename Self, TableType type>
template<::Iterator Iter>
constexpr auto Table<T, Self, type>::insert(ConstIterator, Iter start, Iter end,
                                            Option<size_t> size_hint) requires(CanInsert<IteratorValueType<Iter>, Table>) {
    return result_and_then(insert(move(start), move(end), size_hint), [&](auto&&) {
        return this->end();
    });
}

template<typename T, typename Self, TableType type>
template<typename... Args>
constexpr auto Table<T, Self, type>::emplace(Args&&... args) requires(CreateableFrom<ValueType, Args...>) {
    return insert(create<T>(forward<Args>(args)...));
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr auto Table<T, Self, type>::at(U&& needle) -> Option<Value&>
requires(CanLookup<U, KeyType>) {
    return const_cast<const Table&>(*this).at(forward<U>(needle)).map([](const Value& value) -> Value& {
        return const_cast<Value&>(value);
    });
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr auto Table<T, Self, type>::at(U&& needle) const -> Option<const Value&>
requires(CanLookup<U, KeyType>) {
    auto [hash_high, hash_low] = hash(needle);
    auto entry = find_impl<FindType::PureFind>(hash_high, hash_low, needle);
    return entry.map([&](auto entry) -> const Value& {
        return value(entry.value_index());
    });
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr auto Table<T, Self, type>::find(U&& needle) -> Iterator requires(CanLookup<U, KeyType>) {
    auto [hash_high, hash_low] = hash(needle);
    return find_impl<FindType::PureFind>(hash_high, hash_low, needle)
        .map([this](auto& entry) {
            return iterator_from_entry(entry);
        })
        .value_or(end());
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr auto Table<T, Self, type>::find(U&& needle) const -> ConstIterator requires(CanLookup<U, KeyType>) {
    auto [hash_high, hash_low] = hash(needle);
    return find_impl<FindType::PureFind>(hash_high, hash_low, needle)
        .map([this](auto& entry) {
            return iterator_from_entry(entry);
        })
        .value_or(end());
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr bool Table<T, Self, type>::contains(U&& needle) const requires(CanLookup<U, KeyType>) {
    return !!at(forward<U>(needle));
}

template<typename T, typename Self, TableType type>
constexpr auto Table<T, Self, type>::erase(ConstIterator iterator) -> Option<Value> {
    return erase_entry(entry_from_iterator(iterator));
}

template<typename T, typename Self, TableType type>
constexpr void Table<T, Self, type>::erase(ConstIterator start, ConstIterator end) {
    for (auto it = start; it != end; ++it) {
        erase(it);
    }
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr auto Table<T, Self, type>::erase(U&& needle) -> Option<Value>
requires(CanLookup<U, KeyType>) {
    auto [hash_high, hash_low] = hash(forward<U>(needle));
    return find_impl<FindType::PureFind>(hash_high, hash_low, forward<U>(needle)).and_then([&](auto& entry) {
        return erase_entry(entry);
    });
}

template<typename T, typename Self, TableType type>
constexpr auto Table<T, Self, type>::erase_entry(Entry entry) -> Option<Value> {
    if (!entry.info().present()) {
        return None {};
    }
    auto old_value = Value(move(value(entry.value_index())));
    value_internal(entry.value_index()).~T();
    entry.info().set_tombstone();
    m_size--;
    return old_value;
}

template<typename T, typename Self, TableType type>
constexpr void Table<T, Self, type>::clear() {
    for (auto& value : *this) {
        value.~T();
    }
    delete[] m_groups;
    delete[] m_values;

    m_groups = nullptr;
    m_values = nullptr;
    m_size = m_used_values = m_capacity = 0;
}

template<typename T, typename Self, TableType type>
template<Table<T, Self, type>::FindType find_type, typename U>
constexpr Option<Entry> Table<T, Self, type>::find_impl(uint64_t hash_high, uint8_t hash_low, U&& needle) const
    requires(EqualComparableWith<U, KeyType>) {
    if (group_count() == 0) {
        return None {};
    }

    auto start_group_index = hash_high % group_count();
    auto group_index = start_group_index;
    do {
        auto& group = m_groups[group_index];
        auto present = group.present_entries();
        for (uint8_t index_into_group = 0; index_into_group < GroupInfo::entry_count; index_into_group++) {
            if (present & (1 << index_into_group)) {
                auto& entry = group.entry(index_into_group);
                if (entry.hash_low() == hash_low) {
                    auto value_index = this->value_index(group_index, index_into_group);
                    auto& value = this->value_internal(value_index);
                    if (equal(value, forward<U>(needle))) {
                        return Entry(const_cast<EntryInfo&>(entry), value_index);
                    }
                }
            }
        }

        if ((present | group.tombstone_entries()) != 0xFF) {
            if constexpr (find_type == FindType::FindToInsert) {
                for (uint8_t index_into_group = 0; index_into_group < GroupInfo::entry_count; index_into_group++) {
                    auto& entry = group.entry(index_into_group);
                    if (entry.tombstone()) {
                        return Entry(const_cast<EntryInfo&>(entry), value_index(group_index, index_into_group));
                    }
                }
                for (uint8_t index_into_group = 0; index_into_group < GroupInfo::entry_count; index_into_group++) {
                    auto& entry = group.entry(index_into_group);
                    if (entry.vacant()) {
                        return Entry(const_cast<EntryInfo&>(entry), value_index(group_index, index_into_group));
                    }
                }

                assert(false);
            }
            break;
        }
        group_index++;
        group_index %= group_count();
    } while (start_group_index != group_index);

    return None {};
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr auto Table<T, Self, type>::hash(U&& value) const -> HashSplit requires(HashableLike<U, KeyType>) {
    auto hasher = Hasher {};
    HashForType<U>::hash(hasher, forward<U>(value));
    uint64_t hash = hasher.finish();
    return { hash & ~0x7F, static_cast<uint8_t>(hash & 0x7F) };
}

template<typename T, typename Self, TableType type>
template<typename U>
constexpr bool Table<T, Self, type>::equal(const T& value, U&& other) const requires(EqualComparableWith<U, KeyType>) {
    if constexpr (is_set) {
        return value == other;
    } else {
        return value.first == other;
    }
}

template<typename T, typename Self, TableType type>
constexpr void Table<T, Self, type>::swap(Table& other) {
    ::swap(this->m_groups, other.m_groups);
    ::swap(this->m_values, other.m_values);
    ::swap(this->m_size, other.m_size);
    ::swap(this->m_used_values, other.m_used_values);
    ::swap(this->m_capacity, other.m_capacity);
}
}
