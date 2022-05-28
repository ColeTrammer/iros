#pragma once

// Hash Table implementation modelled from https://www.youtube.com/watch?v=ncHmEUmJZf4.

#include <liim/compare.h>
#include <liim/container.h>
#include <liim/hash/group_info.h>
#include <liim/hash/hashable.h>
#include <liim/hash/hasher.h>

namespace LIIM::Hash::Detail {
enum class FindType {
    PureFind,
    FindToInsert,
};

struct HashSplit {
    uint64_t hash_high;
    uint8_t hash_low;
};

template<typename Table>
class TableIterator {
private:
    Table& m_table;
    size_t m_group_index { 0 };
    uint8_t m_index_into_group { 0 };

    constexpr TableIterator(Table& table, size_t group_index, uint8_t index_into_group)
        : m_table(table), m_group_index(group_index), m_index_into_group(index_into_group) {}

    constexpr TableIterator(Table& table) : m_table(table) {
        if (table.group_count() != 0 && !table.m_groups[0].entry(0).present()) {
            ++*this;
        }
    }

public:
    constexpr operator TableIterator<const Table>() const requires(!IsConst<Table>::value) {
        return TableIterator<const Table>(m_table, m_group_index, m_index_into_group);
    }

    constexpr decltype(auto) operator*() { return m_table.value(m_table.value_index(m_group_index, m_index_into_group)); }

    using ValueType = decltype(*declval<TableIterator>());

    constexpr TableIterator& operator++() {
        for (;;) {
            if (++m_index_into_group == GroupInfo::entry_count) {
                m_index_into_group = 0;
                if (++m_group_index == m_table.group_count()) {
                    break;
                }
            }
            if (m_table.m_groups[m_group_index].entry(m_index_into_group).present()) {
                break;
            }
        }
        return *this;
    }

    constexpr TableIterator operator++(int) {
        auto result = TableIterator(*this);
        ++*this;
        return result;
    }

    constexpr bool operator==(const TableIterator& other) {
        return this->m_group_index == other.m_group_index && this->m_index_into_group == other.m_index_into_group;
    }

    friend Table;
    friend TableIterator<typename RemoveConst<Table>::type>;
};

template<Hashable T>
class Table {
private:
    class Entry {
    public:
        constexpr Entry(EntryInfo& info, size_t value_index) : m_info(&info), m_value_index(value_index) {}

        constexpr EntryInfo& info() { return *m_info; }
        constexpr const EntryInfo& info() const { return *m_info; }
        constexpr size_t value_index() const { return m_value_index; }

    private:
        EntryInfo* m_info { nullptr };
        size_t m_value_index { 0 };
    };

public:
    using ValueType = T;

    constexpr Table() {}
    constexpr Table(const Table&);
    constexpr Table(Table&&);
    constexpr ~Table();

    constexpr Table& operator=(const Table&);
    constexpr Table& operator=(Table&&);

    constexpr bool empty() const { return size() == 0; }
    constexpr size_t size() const { return m_size; }

    using Iterator = TableIterator<Table>;
    using ConstIterator = TableIterator<const Table>;

    friend Iterator;
    friend ConstIterator;

    constexpr Iterator begin() { return Iterator(*this); }
    constexpr ConstIterator begin() const { return ConstIterator(*this); }
    constexpr ConstIterator cbegin() const { return ConstIterator(*this); }

    constexpr Iterator end() { return Iterator(*this, m_capacity, 0); }
    constexpr ConstIterator end() const { return ConstIterator(*this, m_capacity, 0); }
    constexpr ConstIterator cend() const { return ConstIterator(*this, m_capacity, 0); }

    constexpr void clear();

    constexpr Option<T&> insert(const T& to_insert) {
        auto [hash_high, hash_low] = hash(to_insert);

        auto entry = find_impl<FindType::FindToInsert>(hash_high, hash_low, to_insert);
        if (!entry) {
            grow_and_rehash();
            entry = find_impl<FindType::FindToInsert>(hash_high, hash_low, to_insert);
            assert(entry);
        }

        if (entry->info().present()) {
            return value(entry->value_index());
        }

        construct_at(value_pointer_for_init(entry->value_index()), to_insert);
        entry->info().set_present(hash_low);
        m_size++;
        m_used_values++;
        return None {};
    }

    constexpr Option<T&> insert(T&& to_insert) {
        auto [hash_high, hash_low] = hash(to_insert);

        auto entry = find_impl<FindType::FindToInsert>(hash_high, hash_low, to_insert);
        if (!entry) {
            grow_and_rehash();
            entry = find_impl<FindType::FindToInsert>(hash_high, hash_low, to_insert);
            assert(entry);
        }

        if (entry->info().present()) {
            return value(entry->value_index());
        }

        construct_at(value_pointer_for_init(entry->value_index()), move(to_insert));
        entry->info().set_present(hash_low);
        m_size++;
        m_used_values++;
        return None {};
    }

    constexpr Option<T&> find(const T& needle) {
        auto [hash_high, hash_low] = hash(needle);
        auto entry = find_impl<FindType::PureFind>(hash_high, hash_low, needle);
        return entry.map([&](auto entry) -> T& {
            return value(entry.value_index());
        });
    }

    constexpr Option<const T&> find(const T& needle) const {
        auto [hash_high, hash_low] = hash(needle);
        auto entry = find_impl<FindType::PureFind>(hash_high, hash_low, needle);
        return entry.map([&](auto entry) -> const T& {
            return value(entry.value_index());
        });
    }

    constexpr Option<T> erase(ConstIterator iterator);
    constexpr Option<T> erase(const T& needle);

    constexpr void swap(Table& other) {
        ::swap(this->m_groups, other.m_groups);
        ::swap(this->m_values, other.m_values);
        ::swap(this->m_size, other.m_size);
        ::swap(this->m_used_values, other.m_used_values);
        ::swap(this->m_capacity, other.m_capacity);
    }

private:
    explicit constexpr Table(size_t capacity);

    template<FindType find_type, typename U>
    constexpr auto find_impl(uint64_t hash_high, uint8_t hash_low, U&& needle) const -> Option<Entry>;

    constexpr void grow_and_rehash();
    constexpr Option<T> erase(Entry entry);
    constexpr Entry entry_from_iterator(ConstIterator iterator) const;

    constexpr HashSplit hash(const T& value) const {
        auto hasher = Hasher {};
        HashForType<T>::hash(hasher, value);
        uint64_t hash = hasher.finish();
        return { hash & ~0x7F, static_cast<uint8_t>(hash & 0x7F) };
    }

    template<EqualComparable<T> U>
    constexpr bool equal(const T& value, U&& other) const {
        return value == other;
    }

    constexpr size_t group_count() const { return m_capacity; }

    constexpr size_t value_index(size_t group_index, uint8_t index_into_group) const {
        return GroupInfo::entry_count * group_index + index_into_group;
    }

    constexpr T& value(size_t index) { return m_values[index].value; }
    constexpr const T& value(size_t index) const { return m_values[index].value; }

    constexpr T* value_pointer_for_init(size_t index) { return &m_values[index].value; }

    GroupInfo* m_groups { nullptr };
    MaybeUninit<T>* m_values { nullptr };
    size_t m_size { 0 };
    size_t m_used_values { 0 };
    size_t m_capacity { 0 };
};

template<Hashable T>
constexpr Table<T>::Table(const Table& other) {
    for (auto& value : other) {
        insert(value);
    }
}

template<Hashable T>
constexpr Table<T>::Table(Table&& other)
    : m_groups(exchange(other.m_groups, nullptr))
    , m_values(exchange(other.m_values, nullptr))
    , m_size(exchange(other.m_size, 0))
    , m_used_values(exchange(other.m_used_values, 0))
    , m_capacity(exchange(other.m_capacity, 0)) {}

template<Hashable T>
constexpr Table<T>::Table(size_t capacity) : m_capacity(capacity) {
    m_groups = new GroupInfo[capacity];
    m_values = new MaybeUninit<T>[capacity * GroupInfo::entry_count];
}

template<Hashable T>
constexpr Table<T>::~Table() {
    clear();
}

template<Hashable T>
constexpr auto Table<T>::operator=(const Table& other) -> Table& {
    if (this != other) {
        auto new_table = Table(other);
        swap(new_table);
    }
    return *this;
}

template<Hashable T>
constexpr auto Table<T>::operator=(Table&& other) -> Table& {
    if (this != &other) {
        auto new_table = Table(move(other));
        swap(new_table);
    }
    return *this;
}

template<Hashable T>
constexpr void Table<T>::grow_and_rehash() {
    auto new_capacity = 2 * m_capacity ?: 8;
    auto new_table = Table(new_capacity);
    for (auto& value : *this) {
        new_table.insert(move(value));
    }
    this->swap(new_table);
}

template<Hashable T>
constexpr auto Table<T>::entry_from_iterator(ConstIterator iterator) const -> Entry {
    return Entry(m_groups[iterator.m_group_index].entry(iterator.m_index_into_group),
                 value_index(iterator.m_group_index, iterator.m_index_into_group));
}

template<Hashable T>
constexpr Option<T> Table<T>::erase(ConstIterator iterator) {
    return erase(entry_from_iterator(iterator));
}

template<Hashable T>
constexpr Option<T> Table<T>::erase(const T& key) {
    auto [hash_high, hash_low] = hash(key);
    return find_impl<FindType::PureFind>(hash_high, hash_low, key).and_then([&](auto& entry) {
        return erase(entry);
    });
}

template<Hashable T>
constexpr Option<T> Table<T>::erase(Entry entry) {
    if (!entry.info().present()) {
        return None {};
    }
    auto old_value = T(move(value(entry.value_index())));
    value(entry.value_index()).~T();
    entry.info().set_tombstone();
    m_size--;
    return old_value;
}

template<Hashable T>
constexpr void Table<T>::clear() {
    for (auto& value : *this) {
        value.~T();
    }
    delete[] m_groups;
    delete[] m_values;

    m_groups = nullptr;
    m_values = nullptr;
    m_size = m_used_values = m_capacity = 0;
}

template<Hashable T>
template<FindType find_type, typename U>
constexpr auto Table<T>::find_impl(uint64_t hash_high, uint8_t hash_low, U&& needle) const -> Option<Entry> {
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
                    auto& value = this->value(value_index);
                    if (value == forward<U>(needle)) {
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
}
