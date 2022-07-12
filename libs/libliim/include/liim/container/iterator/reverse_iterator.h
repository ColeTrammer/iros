#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/swap_iterator_contents.h>

namespace LIIM::Container::Iterators {
template<RandomAccessIterator Iter>
class ReverseIterator {
public:
    using ValueType = IteratorValueType<Iter>;

    constexpr explicit ReverseIterator(Iter iter) : m_iterator(move(iter)) {}

    constexpr Iter base() const { return m_iterator; }

    constexpr decltype(auto) operator*() { return m_iterator[-1]; }

    constexpr decltype(auto) operator[](ssize_t index) const { return m_iterator[-1 - index]; }

    constexpr ReverseIterator& operator++() {
        --m_iterator;
        return *this;
    }
    constexpr ReverseIterator& operator--() {
        ++m_iterator;
        return *this;
    }

    constexpr ReverseIterator operator++(int) { return ReverseIterator(m_iterator--); }
    constexpr ReverseIterator operator--(int) { return ReverseIterator(m_iterator++); }

    constexpr ReverseIterator operator+(ssize_t n) const { return ReverseIterator(m_iterator - n); }
    constexpr ReverseIterator operator-(ssize_t n) const { return ReverseIterator(m_iterator + n); }

    constexpr ssize_t operator-(const ReverseIterator& other) const { return other.base() - this->base(); }

    constexpr ReverseIterator& operator+=(ssize_t n) {
        m_iterator -= n;
        return *this;
    }
    constexpr ReverseIterator& operator-=(ssize_t n) {
        m_iterator += n;
        return *this;
    }

    constexpr bool operator==(const ReverseIterator& other) const { return this->m_iterator == other.m_iterator; }
    constexpr auto operator<=>(const ReverseIterator& other) const { return other.m_iterator <=> this->m_iterator; }

    constexpr void swap_contents(ReverseIterator other) requires(MutableIterator<Iter>) {
        auto it = this->m_iterator - 1;
        auto jt = other.m_iterator - 1;
        swap_iterator_contents(it, jt);
    }

private:
    Iter m_iterator;
};
}

using LIIM::Container::Iterators::ReverseIterator;
