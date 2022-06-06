#pragma once

#include <liim/format.h>
#include <liim/hash/table.h>
#include <liim/initializer_list.h>

namespace LIIM::Hash {
template<Hashable T>
class Set {
private:
    using Table = Detail::Table<T, Detail::TableType::Set>;

public:
    constexpr Set() {}
    constexpr Set(Set&&) = default;

    static constexpr Set create(std::initializer_list<T> list);
    template<Iterator Iter>
    static constexpr Set create(Iter begin, Iter end, Option<size_t> known_size = {});

    constexpr Set clone() const requires(Cloneable<T>) { return Set(m_table.clone()); }

    constexpr Set& operator=(Set&&) = default;
    constexpr Set& assign(std::initializer_list<T> list);

    using Iterator = Table::Iterator;
    using ConstIterator = Table::ConstIterator;

    constexpr auto begin() { return m_table.begin(); }
    constexpr auto begin() const { return m_table.begin(); }
    constexpr auto cbegin() const { return m_table.cbegin(); }

    constexpr auto end() { return m_table.end(); }
    constexpr auto end() const { return m_table.end(); }
    constexpr auto cend() const { return m_table.cend(); }

    constexpr bool empty() const { return m_table.empty(); }
    constexpr size_t size() const { return m_table.size(); }

    constexpr void clear() { m_table.clear(); }

    constexpr Option<T> erase(ConstIterator position) { return m_table.erase(position); }
    constexpr void erase(ConstIterator start, ConstIterator end);

    template<Detail::CanLookup<T> U>
    constexpr Option<T> erase(U&& key) {
        return m_table.erase(forward<U>(key));
    }

    template<Detail::CanInsertIntoSet<T> U>
    constexpr Option<T&> insert(U&& value) {
        return m_table.insert(forward<U>(value));
    }
    template<Detail::CanInsertIntoSet<T> U>
    constexpr Iterator insert(ConstIterator hint, U&& value) {
        return m_table.insert(hint, forward<U>(value));
    }

    template<::Iterator Iter>
    constexpr void insert(Iter begin, Iter end, Option<size_t> known_size = {}) {
        return m_table.insert(move(begin), move(end), known_size);
    }

    template<::Iterator Iter>
    constexpr Iterator insert(ConstIterator hint, Iter begin, Iter end, Option<size_t> known_size = {}) {
        return m_table.insert(hint, move(begin), move(end), known_size);
    }

    constexpr void insert(std::initializer_list<T> list);

    template<typename... Args>
    constexpr Option<T&> emplace(Args&&... args) {
        return m_table.insert(create<T>(forward<Args>(args)...));
    }

    template<Detail::CanLookup<T> U>
    constexpr auto at(U&& needle) {
        return m_table.at(forward<U>(needle));
    }
    template<Detail::CanLookup<T> U>
    constexpr auto at(U&& needle) const {
        return m_table.at(forward<U>(needle));
    }

    template<Detail::CanLookup<T> U>
    constexpr auto find(U&& needle) {
        return m_table.find(forward<U>(needle));
    }
    template<Detail::CanLookup<T> U>
    constexpr auto find(U&& needle) const {
        return m_table.find(forward<U>(needle));
    }

    template<Detail::CanLookup<T> U>
    constexpr bool contains(U&& needle) const {
        return !!m_table.at(forward<U>(needle));
    }

    constexpr void swap(Set& other) { this->m_table.swap(other.m_table); }

    constexpr bool operator==(const Set& other) const requires(EqualComparable<T>);

private:
    explicit constexpr Set(Table&& table) : m_table(move(table)) {}

    Table m_table;
};

template<Hashable T>
constexpr auto Set<T>::create(std::initializer_list<T> list) -> Set {
    return Set::create(list.begin(), list.end());
}

template<Hashable T>
template<Iterator Iter>
constexpr auto Set<T>::create(Iter begin, Iter end, Option<size_t> known_size) -> Set {
    Set result;
    result.insert(move(begin), move(end), known_size);
    return result;
}

template<Hashable T>
constexpr auto Set<T>::assign(std::initializer_list<T> list) -> Set& {
    return *this = Set::create(list);
}

template<Hashable T>
constexpr void Set<T>::erase(ConstIterator start, ConstIterator end) {
    for (auto it = start; it != end; ++it) {
        erase(it);
    }
}

template<Hashable T>
constexpr void Set<T>::insert(std::initializer_list<T> list) {
    insert(list.begin(), list.end());
}

template<Hashable T>
constexpr bool Set<T>::operator==(const Set& other) const requires(EqualComparable<T>) {
    if (this->size() != other.size()) {
        return false;
    }
    for (auto& value : *this) {
        if (!other.contains(value)) {
            return false;
        }
    }
    return true;
}

template<typename T>
constexpr Set<T> make_hash_set(std::initializer_list<T> list) {
    return Set<T>::create(list);
}

template<Iterator Iter>
constexpr auto make_hash_set(Iter start, Iter end, Option<size_t> known_size = {}) {
    using SetType = Set<decay_t<IteratorValueType<Iter>>>;
    return SetType::create(move(start), move(end), known_size);
}

template<Container C>
constexpr auto collect_hash_set(C&& container) {
    using SetType = Set<decay_t<ContainerValueType<C>>>;
    return collect<SetType>(forward<C>(container));
}
}

namespace LIIM::Format {
template<Formattable T>
struct Formatter<LIIM::Hash::Set<T>> {
    constexpr void parse(FormatParseContext& context) { m_formatter.parse(context); }

    void format(const LIIM::Hash::Set<T>& set, FormatContext& context) {
        context.put("{ ");
        bool first = true;
        for (auto& item : set) {
            if (!first) {
                context.put(", ");
            }
            m_formatter.format(item, context);
            first = false;
        }
        context.put(" }");
    }

    Formatter<T> m_formatter;
};
}
