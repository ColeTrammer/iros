#pragma once

#include <liim/container/concepts.h>

namespace LIIM::Container::Iterators {
template<Iterator Iter>
class MoveIterator {
public:
    using BaseValueType = IteratorTraits<Iter>::ValueType;
    static constexpr bool is_reference = IsReference<BaseValueType>::value;

    using ValueType = Conditional<is_reference, typename RemoveReference<BaseValueType>::type&&, BaseValueType>::type;

    constexpr explicit MoveIterator(Iter iter) : m_iterator(move(iter)) {}

    constexpr Iter base() const { return m_iterator; }

    constexpr ValueType operator*() { return move(*m_iterator); }

    constexpr MoveIterator& operator++() {
        ++m_iterator;
        return *this;
    }

    constexpr MoveIterator operator++(int) const { return MoveIterator(m_iterator++); }

    constexpr bool operator==(const MoveIterator& other) const { return this->m_iterator == other.m_iterator; }

private:
    Iter m_iterator;
};
}

using LIIM::Container::Iterators::MoveIterator;
