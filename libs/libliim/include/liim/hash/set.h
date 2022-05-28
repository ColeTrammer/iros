#pragma once

#include <liim/format.h>
#include <liim/hash/table.h>
#include <liim/initializer_list.h>

namespace LIIM::Hash {
template<Hashable T>
class Set {
public:
    constexpr Set() {}
    constexpr Set(const Set&) = default;
    constexpr Set(Set&&) = default;
    constexpr Set(std::initializer_list<T> list);

    constexpr Set& operator=(const Set&) = default;
    constexpr Set& operator=(Set&&) = default;
    constexpr Set& operator=(std::initializer_list<T> list);

    using Iterator = Detail::Table<T>::Iterator;
    using ConstIterator = Detail::Table<T>::ConstIterator;

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

    template<Detail::CanInsert<T> U>
    constexpr Option<T&> insert(U&& value) {
        return m_table.insert(forward<U>(value));
    }

    template<::Iterator Iter>
    constexpr void insert(Iter begin, Iter end);
    template<Container C>
    constexpr void insert(C&& container) requires(!Detail::CanInsert<C, T>);
    constexpr void insert(std::initializer_list<T> list);

    template<typename... Args>
    constexpr Option<T&> emplace(Args&&... args) {
        return m_table.insert(T(forward<Args>(args)...));
    }

    template<typename U>
    constexpr Option<T&> find(U&& needle) {
        return m_table.find(needle);
    }
    template<typename U>
    constexpr Option<const T&> find(U&& needle) const {
        return m_table.find(needle);
    }

    constexpr bool contains(const T& needle) const { return !!m_table.find(needle); }

    constexpr void swap(Set& other) { this->m_table.swap(other.m_table); }

    constexpr bool operator==(const Set& other) const requires(EqualComparable<T>);

private:
    Detail::Table<T> m_table;
};

template<Hashable T>
constexpr Set<T>::Set(std::initializer_list<T> list) {
    insert(list.begin(), list.end());
}

template<Hashable T>
constexpr auto Set<T>::operator=(std::initializer_list<T> list) -> Set& {
    auto result = Set(list);
    swap(result);
    return *this;
}

template<Hashable T>
constexpr void Set<T>::erase(ConstIterator start, ConstIterator end) {
    for (auto it = start; it != end; ++it) {
        erase(it);
    }
}

template<Hashable T>
template<Iterator Iter>
constexpr void Set<T>::insert(Iter begin, Iter end) {
    return insert(iterator_container(move(begin), move(end)));
}

template<Hashable T>
constexpr void Set<T>::insert(std::initializer_list<T> list) {
    insert(list.begin(), list.end());
}

template<Hashable T>
template<Container C>
constexpr void Set<T>::insert(C&& container) requires(!Detail::CanInsert<C, T>) {
    using ValueType = IteratorTraits<decltype(container.begin())>::ValueType;
    constexpr bool is_const = IsConst<typename RemoveReference<ValueType>::type>::value;

    if constexpr (!is_const && !IsLValueReference<C>::value && !IsConst<decay_t<C>>::value) {
        for (auto&& value : move_elements(move(container))) {
            insert(move(value));
        }
    } else {
        for (const auto& value : container) {
            insert(value);
        }
    }
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
