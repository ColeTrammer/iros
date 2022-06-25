#pragma once

#include <liim/container/hash/table.h>
#include <liim/format.h>
#include <liim/initializer_list.h>

namespace LIIM::Container::Hash {
template<Hashable T>
class Set : public Detail::Table<T, Set<T>, Detail::TableType::Set> {
public:
    constexpr bool operator==(const Set& other) const requires(EqualComparable<T>);
};

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
constexpr auto make_hash_set(std::initializer_list<T> list) {
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
struct Formatter<LIIM::Container::Hash::Set<T>> {
    constexpr void parse(FormatParseContext& context) { m_formatter.parse(context); }

    void format(const LIIM::Container::Hash::Set<T>& set, FormatContext& context) {
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
